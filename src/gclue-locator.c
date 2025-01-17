/* vim: set et ts=8 sw=8: */
/* gclue-locator.c
 *
 * Copyright 2013 Red Hat, Inc.
 * Copyright © 2022,2023 Oracle and/or its affiliates.
 *
 * Geoclue is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * Geoclue is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Geoclue; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Authors: Zeeshan Ali (Khattak) <zeeshanak@gnome.org>
 *          Maciej S. Szmigiero <maciej.szmigiero@oracle.com>
 */

#include "config.h"

#include <glib/gi18n.h>

#include "gclue-locator.h"

#include "gclue-static-source.h"
#include "gclue-wifi.h"
#include "gclue-config.h"

#if GCLUE_USE_3G_SOURCE
#include "gclue-3g.h"
#endif

#if GCLUE_USE_CDMA_SOURCE
#include "gclue-cdma.h"
#endif

#if GCLUE_USE_MODEM_GPS_SOURCE
#include "gclue-modem-gps.h"
#endif

#if GCLUE_USE_NMEA_SOURCE
#include "gclue-nmea-source.h"
#endif

#if GCLUE_USE_HYBRIS_SOURCE
#include "gclue-hybris-source.h"
#endif

/* This class is like a master location source that hides all individual
 * location sources from rest of the code
 */

static GClueLocationSourceStartResult
gclue_locator_start (GClueLocationSource *source);
static GClueLocationSourceStopResult
gclue_locator_stop (GClueLocationSource *source);

struct _GClueLocatorPrivate
{
        GList *sources;
        GList *active_sources;

        GClueAccuracyLevel accuracy_level;
        gboolean priority_source_lock;
        guint64 priority_source_lock_timestamp;
};

G_DEFINE_TYPE_WITH_CODE (GClueLocator,
                         gclue_locator,
                         GCLUE_TYPE_LOCATION_SOURCE,
                         G_ADD_PRIVATE (GClueLocator))

enum
{
        PROP_0,
        PROP_ACCURACY_LEVEL,
        LAST_PROP
};

static GParamSpec *gParamSpecs[LAST_PROP];

#define MAX_SPEED        500       /* Meters per second */
#define MAX_LOCATION_AGE (30 * 60) /* Seconds. */
#define MAX_PRIORITY_SOURCE_AGE         30        /* Seconds. */
#define PRIORITY_ACCURACY_THRESHOLD 20        /* Meters */

static void
set_location (GClueLocator  *locator,
              GClueLocationSource *source)
{
        GClueLocation *cur_location;
        GClueLocation *location;
        const char *src_name = NULL;
        gboolean update_priority_source = FALSE;

        location = gclue_location_source_get_location (source);
        src_name = G_OBJECT_TYPE_NAME (source);

        if (gclue_location_get_accuracy (location) ==
            GCLUE_LOCATION_ACCURACY_UNKNOWN) {
                /* If we do not know the accuracy, discard the update */
                g_debug ("Discarding %s location with unknown accuracy",
                         src_name);
                return;
        }

        cur_location = gclue_location_source_get_location
                        (GCLUE_LOCATION_SOURCE (locator));

        if (cur_location != NULL) {
            guint64 cur_timestamp, new_timestamp;
            double dist, speed;

            cur_timestamp = gclue_location_get_timestamp (cur_location);
            new_timestamp = gclue_location_get_timestamp (location);
            if (new_timestamp < cur_timestamp) {
                    g_debug ("New %s location older than current, ignoring.",
                             src_name);
                    return;
            }

            dist = gclue_location_get_distance_from (location, cur_location);
            if (new_timestamp > cur_timestamp) {
                guint64 age = new_timestamp - cur_timestamp;

                if (age < MAX_LOCATION_AGE) {
                    speed = dist / age;
                } else {
                    /* The previous location is too old?
                     * Force the speed to be within the allowed range then.
                     */
                    speed = 0;
                }
            } else {
                speed = G_MAXDOUBLE;
            }

            update_priority_source = gclue_location_source_get_priority_source (source) &&
                                     (gclue_location_get_accuracy (location) < PRIORITY_ACCURACY_THRESHOLD);

            if (update_priority_source) {
                     if (!locator->priv->priority_source_lock)
                              g_debug ("Enabling Priority Source Lock");
                     locator->priv->priority_source_lock = TRUE;
                     locator->priv->priority_source_lock_timestamp = new_timestamp;
            } else if (locator->priv->priority_source_lock &&
                       (new_timestamp - locator->priv->priority_source_lock_timestamp) >= MAX_PRIORITY_SOURCE_AGE) {
                     g_debug ("Priority Source Lock no longer active");
                     locator->priv->priority_source_lock = FALSE;
            }

            if (locator->priv->priority_source_lock &&
                !gclue_location_source_get_priority_source (source)) {
                     g_debug ("Priority Source Lock (age %lu) active, ignoring new %s location",
                              new_timestamp - locator->priv->priority_source_lock_timestamp, src_name);
                     return;
            }

            if (update_priority_source) {
                     /* A priority source is updating, let it though */
                     g_debug ("Priority Source Lock Active");
            } else if ((dist <= gclue_location_get_accuracy (location) ||
                       speed > MAX_SPEED) &&
                       gclue_location_get_accuracy (location) >
                       gclue_location_get_accuracy (cur_location)) {
                    /* We only take the new location if either the previous one
                     * lies outside its accuracy circle and was reachable with
                     * a reasonable speed, OR it is more or as accurate as
                     * the previous one.
                     */
                    g_debug ("Ignoring less accurate new %s location", src_name);
                    return;
            }
        }

        g_debug ("New location available from %s", src_name);
        gclue_location_source_set_location (GCLUE_LOCATION_SOURCE (locator),
                                            location);
}

static gint
compare_accuracy_level (GClueLocationSource *src_a,
                        GClueLocationSource *src_b)
{
        GClueAccuracyLevel level_a, level_b;

        level_a = gclue_location_source_get_available_accuracy_level (src_a);
        level_b = gclue_location_source_get_available_accuracy_level (src_b);

        return (level_b - level_a);
}

static void
refresh_available_accuracy_level (GClueLocator *locator)
{
        GClueAccuracyLevel new, existing;

        /* Sort the sources according to their accuracy level so that the head
         * of the list will have the highest level. The goal is to start the
         * most accurate source first and when all sources are already active
         * for an app, a second app to get the most accurate location only.
         */
        locator->priv->sources = g_list_sort
                        (locator->priv->sources,
                         (GCompareFunc) compare_accuracy_level);

        new = gclue_location_source_get_available_accuracy_level
                        (GCLUE_LOCATION_SOURCE (locator->priv->sources->data));

        existing = gclue_location_source_get_available_accuracy_level
                        (GCLUE_LOCATION_SOURCE (locator));

        if (new != existing)
                g_object_set (G_OBJECT (locator),
                              "available-accuracy-level", new,
                              NULL);
}

static void
on_location_changed (GObject    *gobject,
                     GParamSpec *pspec,
                     gpointer    user_data)
{
        GClueLocator *locator = GCLUE_LOCATOR (user_data);
        GClueLocationSource *source = GCLUE_LOCATION_SOURCE (gobject);

        set_location (locator, source);
}

static gboolean
is_source_active (GClueLocator        *locator,
                  GClueLocationSource *src)
{
        return (g_list_find (locator->priv->active_sources, src) != NULL);
}

static void
start_source (GClueLocator        *locator,
              GClueLocationSource *src)
{
        GClueLocation *location;

        g_signal_connect (G_OBJECT (src),
                          "notify::location",
                          G_CALLBACK (on_location_changed),
                          locator);

        location = gclue_location_source_get_location (src);
        if (gclue_location_source_get_active (src) && location != NULL)
                set_location (locator, src);

        gclue_location_source_start (src);
}

static void
on_avail_accuracy_level_changed (GObject    *gobject,
                                 GParamSpec *pspec,
                                 gpointer    user_data)
{
        GClueLocationSource *src = GCLUE_LOCATION_SOURCE (gobject);
        GClueLocator *locator = GCLUE_LOCATOR (user_data);
        GClueLocatorPrivate *priv = locator->priv;
        GClueAccuracyLevel level;
        gboolean active;

        refresh_available_accuracy_level (locator);

        active = gclue_location_source_get_active
                (GCLUE_LOCATION_SOURCE (locator));
        if (!active)
                return;

        level = gclue_location_source_get_available_accuracy_level (src);
        if (level != GCLUE_ACCURACY_LEVEL_NONE &&
            priv->accuracy_level >= level &&
            !is_source_active (locator, src)) {
                start_source (locator, src);

                priv->active_sources =
                        g_list_append (locator->priv->active_sources, src);
        } else if ((level == GCLUE_ACCURACY_LEVEL_NONE ||
                    priv->accuracy_level < level) &&
                   is_source_active (locator, src)) {
                g_signal_handlers_disconnect_by_func (G_OBJECT (src),
                                                      G_CALLBACK (on_location_changed),
                                                      locator);
                gclue_location_source_stop (src);
                priv->active_sources = g_list_remove (priv->active_sources,
                                                      src);
        }
}

static void
reset_time_threshold (GClueLocator        *locator,
                      GClueLocationSource *source,
                      guint                value)
{
        GClueMinUINT *threshold;

        threshold = gclue_location_source_get_time_threshold (source);

        gclue_min_uint_add_value (threshold, value, G_OBJECT (locator));
}

static void
on_time_threshold_changed (GObject    *gobject,
                           GParamSpec *pspec,
                           gpointer    user_data)
{
        GClueMinUINT *threshold = GCLUE_MIN_UINT (gobject);
        GClueLocator *locator = GCLUE_LOCATOR (user_data);
        guint value = gclue_min_uint_get_value (threshold);
        GList *node;

        for (node = locator->priv->sources; node != NULL; node = node->next) {
                reset_time_threshold (locator,
                                      GCLUE_LOCATION_SOURCE (node->data),
                                      value);
        }
}

static void
gclue_locator_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
        GClueLocator *locator = GCLUE_LOCATOR (object);

        switch (prop_id) {
        case PROP_ACCURACY_LEVEL:
                g_value_set_enum (value, locator->priv->accuracy_level);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
gclue_locator_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
        GClueLocator *locator = GCLUE_LOCATOR (object);

        switch (prop_id) {
        case PROP_ACCURACY_LEVEL:
                locator->priv->accuracy_level = g_value_get_enum (value);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
gclue_locator_finalize (GObject *gsource)
{
        GClueLocator *locator = GCLUE_LOCATOR (gsource);
        GClueLocatorPrivate *priv = locator->priv;
        GList *node;
        GClueMinUINT *threshold;

        threshold = gclue_location_source_get_time_threshold
                        (GCLUE_LOCATION_SOURCE (locator));
        g_signal_handlers_disconnect_by_func
                (G_OBJECT (threshold),
                 G_CALLBACK (on_time_threshold_changed),
                 locator);

        for (node = locator->priv->sources; node != NULL; node = node->next) {
                g_signal_handlers_disconnect_by_func
                        (G_OBJECT (node->data),
                         G_CALLBACK (on_avail_accuracy_level_changed),
                         locator);
        }
        for (node = locator->priv->active_sources; node != NULL; node = node->next) {
                g_signal_handlers_disconnect_by_func
                        (G_OBJECT (node->data),
                         G_CALLBACK (on_location_changed),
                         locator);
                gclue_location_source_stop (GCLUE_LOCATION_SOURCE (node->data));
        }
        g_list_free_full (priv->sources, g_object_unref);
        priv->sources = NULL;
        g_list_free (priv->active_sources);
        priv->active_sources = NULL;

        G_OBJECT_CLASS (gclue_locator_parent_class)->finalize (gsource);
}

static void
gclue_locator_constructed (GObject *object)
{
        GClueLocator *locator = GCLUE_LOCATOR (object);
        GClueLocationSource *submit_source = NULL;
        GClueConfig *gconfig = gclue_config_get_singleton ();
        GClueWifi *wifi = NULL;
        GList *node;
        GClueMinUINT *threshold;

        G_OBJECT_CLASS (gclue_locator_parent_class)->constructed (object);

#if GCLUE_USE_3G_SOURCE
        if (gclue_config_get_enable_3g_source (gconfig)) {
                GClue3G *source = gclue_3g_get_singleton (locator->priv->accuracy_level);
                locator->priv->sources = g_list_append (locator->priv->sources,
                                                        source);
        }
#endif
#if GCLUE_USE_CDMA_SOURCE
        if (gclue_config_get_enable_cdma_source (gconfig)) {
                GClueCDMA *cdma = gclue_cdma_get_singleton ();
                locator->priv->sources = g_list_append (locator->priv->sources,
                                                        cdma);
        }
#endif
        if (gclue_config_get_enable_wifi_source (gconfig)) {
                wifi = gclue_wifi_get_singleton (locator->priv->accuracy_level);
        } else {
                if (gclue_config_get_enable_static_source (gconfig)) {
                        g_debug ("Disabling GeoIP-only source since static source is enabled");
                } else {
                        /* City-level accuracy will give us GeoIP-only source */
                        wifi = gclue_wifi_get_singleton (GCLUE_ACCURACY_LEVEL_CITY);
                }
        }
        if (wifi) {
                locator->priv->sources = g_list_append (locator->priv->sources,
                                                        wifi);
        }
#if GCLUE_USE_MODEM_GPS_SOURCE
        if (gclue_config_get_enable_modem_gps_source (gconfig)) {
                GClueModemGPS *gps = gclue_modem_gps_get_singleton ();
                locator->priv->sources = g_list_append (locator->priv->sources,
                                                        gps);
                if (!submit_source) {
                        submit_source = GCLUE_LOCATION_SOURCE (gps);
                }
        }
#endif
#if GCLUE_USE_NMEA_SOURCE
        if (gclue_config_get_enable_nmea_source (gconfig)) {
                GClueNMEASource *nmea = gclue_nmea_source_get_singleton ();
                locator->priv->sources = g_list_append (locator->priv->sources,
                                                        nmea);
                if (!submit_source) {
                        submit_source = GCLUE_LOCATION_SOURCE (nmea);
                }

        }
#endif
#if GCLUE_USE_HYBRIS_SOURCE
        if (gclue_config_get_enable_hybris_source (gconfig)) {
                GClueHybrisSource *hybris = gclue_hybris_source_get_singleton ();
                locator->priv->sources = g_list_append (locator->priv->sources,
                                                        hybris);
        }
#endif

        if (gclue_config_get_enable_static_source (gconfig)) {
                GClueStaticSource *static_source;

                static_source = gclue_static_source_get_singleton
                        (locator->priv->accuracy_level);
                locator->priv->sources = g_list_append (locator->priv->sources,
                                                        static_source);
        }

        for (node = locator->priv->sources; node != NULL; node = node->next) {
                g_signal_connect (G_OBJECT (node->data),
                                  "notify::available-accuracy-level",
                                  G_CALLBACK (on_avail_accuracy_level_changed),
                                  locator);

                if (submit_source != NULL && GCLUE_IS_WEB_SOURCE (node->data))
                        gclue_web_source_set_submit_source
                                (GCLUE_WEB_SOURCE (node->data), submit_source);
        }

        threshold = gclue_location_source_get_time_threshold
                        (GCLUE_LOCATION_SOURCE (locator));
        g_signal_connect (G_OBJECT (threshold),
                          "notify::value",
                          G_CALLBACK (on_time_threshold_changed),
                          locator);
        refresh_available_accuracy_level (locator);
}

static void
gclue_locator_class_init (GClueLocatorClass *klass)
{
        GClueLocationSourceClass *source_class = GCLUE_LOCATION_SOURCE_CLASS (klass);
        GObjectClass *object_class;

        source_class->start = gclue_locator_start;
        source_class->stop = gclue_locator_stop;

        object_class = G_OBJECT_CLASS (klass);
        object_class->get_property = gclue_locator_get_property;
        object_class->set_property = gclue_locator_set_property;
        object_class->finalize = gclue_locator_finalize;
        object_class->constructed = gclue_locator_constructed;

        gParamSpecs[PROP_ACCURACY_LEVEL] = g_param_spec_enum ("accuracy-level",
                                                              "AccuracyLevel",
                                                              "Accuracy level",
                                                              GCLUE_TYPE_ACCURACY_LEVEL,
                                                              GCLUE_ACCURACY_LEVEL_CITY,
                                                              G_PARAM_READWRITE |
                                                              G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_property (object_class,
                                         PROP_ACCURACY_LEVEL,
                                         gParamSpecs[PROP_ACCURACY_LEVEL]);
}

static void
gclue_locator_init (GClueLocator *locator)
{
        locator->priv = gclue_locator_get_instance_private (locator);
        locator->priv->priority_source_lock = FALSE;
        locator->priv->priority_source_lock_timestamp = 0;
}

static GClueLocationSourceStartResult
gclue_locator_start (GClueLocationSource *source)
{
        GClueLocationSourceClass *base_class;
        GClueLocator *locator;
        GList *node;
        GClueLocationSourceStartResult base_result;

        g_return_val_if_fail (GCLUE_IS_LOCATOR (source),
                              GCLUE_LOCATION_SOURCE_START_RESULT_FAILED);
        locator = GCLUE_LOCATOR (source);

        base_class = GCLUE_LOCATION_SOURCE_CLASS (gclue_locator_parent_class);
        base_result = base_class->start (source);
        if (base_result != GCLUE_LOCATION_SOURCE_START_RESULT_OK)
                return base_result;

        for (node = locator->priv->sources; node != NULL; node = node->next) {
                GClueLocationSource *src = GCLUE_LOCATION_SOURCE (node->data);
                GClueAccuracyLevel level;

                level = gclue_location_source_get_available_accuracy_level (src);
                if (level > locator->priv->accuracy_level ||
                    level == GCLUE_ACCURACY_LEVEL_NONE) {
                        g_debug ("Not starting %s (accuracy level: %u). "
                                 "Requested accuracy level: %u.",
                                 G_OBJECT_TYPE_NAME (src),
                                 level,
                                 locator->priv->accuracy_level);
                        continue;
                }

                locator->priv->active_sources = g_list_append (locator->priv->active_sources,
                                                               src);

                start_source (locator, src);
        }

        return base_result;
}

static GClueLocationSourceStopResult
gclue_locator_stop (GClueLocationSource *source)
{
        GClueLocationSourceClass *base_class;
        GClueLocator *locator;
        GList *node;
        GClueLocationSourceStopResult base_result;

        g_return_val_if_fail (GCLUE_IS_LOCATOR (source), FALSE);
        locator = GCLUE_LOCATOR (source);

        base_class = GCLUE_LOCATION_SOURCE_CLASS (gclue_locator_parent_class);
        base_result = base_class->stop (source);
        if (base_result == GCLUE_LOCATION_SOURCE_STOP_RESULT_STILL_USED)
                return base_result;

        for (node = locator->priv->active_sources; node != NULL; node = node->next) {
                GClueLocationSource *src = GCLUE_LOCATION_SOURCE (node->data);

                g_signal_handlers_disconnect_by_func (G_OBJECT (src),
                                                      G_CALLBACK (on_location_changed),
                                                      locator);
                gclue_location_source_stop (src);
                g_debug ("Requested %s to stop", G_OBJECT_TYPE_NAME (src));
        }

        g_list_free (locator->priv->active_sources);
        locator->priv->active_sources = NULL;
        return base_result;
}

GClueLocator *
gclue_locator_new (GClueAccuracyLevel level)
{
        GClueAccuracyLevel accuracy_level = level;

        if (accuracy_level == GCLUE_ACCURACY_LEVEL_COUNTRY)
                /* There is no source that provides country-level accuracy.
                 * Since Wifi (as geoip) source is the best we can do, accuracy
                 * really is country-level many times from this source and its
                 * doubtful app (or user) will mind being given slighly more
                 * accurate location, lets just map this to city-level accuracy.
                 */
                accuracy_level = GCLUE_ACCURACY_LEVEL_CITY;

        return g_object_new (GCLUE_TYPE_LOCATOR,
                             "accuracy-level", accuracy_level,
                             "compute-movement", FALSE,
                             NULL);
}

GClueAccuracyLevel
gclue_locator_get_accuracy_level (GClueLocator *locator)
{
        g_return_val_if_fail (GCLUE_IS_LOCATOR (locator),
                              GCLUE_ACCURACY_LEVEL_NONE);

        return locator->priv->accuracy_level;
}

/**
 * gclue_locator_get_time_threshold
 * @locator: a #GClueLocator
 *
 * Returns: The current time-threshold in seconds.
 **/
guint
gclue_locator_get_time_threshold (GClueLocator *locator)
{
        GClueMinUINT *threshold;

        g_return_val_if_fail (GCLUE_IS_LOCATOR (locator), 0);

        threshold = gclue_location_source_get_time_threshold
                        (GCLUE_LOCATION_SOURCE (locator));

        return gclue_min_uint_get_value (threshold);
}

/**
 * gclue_locator_set_time_threshold
 * @locator: a #GClueLocator
 * @value: The new threshold value
 *
 * Sets the time-threshold to @value.
 *
 * Unlike other (real) location sources, Locator instances are unique for each
 * client application. Which means we only need just one time-threshold value
 * and hence the reason we have these getter and setters, instead of making use
 * of the #GClueLocationSource:time-threshold property.
 **/
void
gclue_locator_set_time_threshold (GClueLocator *locator,
                                  guint         value)
{
        g_return_if_fail (GCLUE_IS_LOCATOR (locator));

        reset_time_threshold (locator,
                              GCLUE_LOCATION_SOURCE (locator),
                              value);
}
