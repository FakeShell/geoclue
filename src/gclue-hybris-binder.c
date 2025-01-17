/* vim: set et ts=8 sw=8: */
/*
    Copyright (C) 2015 Jolla Ltd.
    Copyright (C) 2018 Matti Lehtimäki <matti.lehtimaki@gmail.com>
    Copyright (C) 2024 Bardia Moshiri <fakeshell@bardia.tech>

    This file is part of geoclue-hybris.

    Geoclue-hybris is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License.
*/

#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <glib.h>
#include <strings.h>
#include <sys/time.h>
#include <dbus/dbus.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <netdb.h>

#include "gclue-hybris-binder.h"
#include "gclue-config.h"

#define GNSS_BINDER_DEFAULT_DEV  "/dev/hwbinder"

static void
gclue_hybris_interface_init (GClueHybrisInterface *iface);

struct _GClueHybrisBinderPrivate {
    gulong m_death_id;
    char *m_fqname;
    char *m_fqname_2_0;
    GBinderServiceManager *m_sm;

    GBinderClient *m_clientGnss;
    GBinderClient *m_clientGnss_2_0;

    GBinderRemoteObject *m_remoteGnss;
    GBinderLocalObject *m_callbackGnss;
    GBinderRemoteObject *m_remoteGnss_2_0;

    GBinderClient *m_clientGnssDebug;
    GBinderRemoteObject *m_remoteGnssDebug;

    GBinderClient *m_clientGnssNi;
    GBinderRemoteObject *m_remoteGnssNi;
    GBinderLocalObject *m_callbackGnssNi;

    GBinderClient *m_clientGnssXtra;
    GBinderRemoteObject *m_remoteGnssXtra;
    GBinderLocalObject *m_callbackGnssXtra;

    GBinderClient *m_clientAGnss;
    GBinderRemoteObject *m_remoteAGnss;
    GBinderLocalObject *m_callbackAGnss;

    GBinderClient *m_clientAGnssRil;
    GBinderRemoteObject *m_remoteAGnssRil;
    GBinderLocalObject *m_callbackAGnssRil;

    int m_gnss2Available;
};

G_DEFINE_TYPE_WITH_CODE (GClueHybrisBinder, gclue_hybris_binder, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GCLUE_TYPE_HYBRIS,
                                                gclue_hybris_interface_init)
                         G_ADD_PRIVATE (GClueHybrisBinder))

enum {
        SET_LOCATION,
        SIGNAL_LAST
};

static guint signals[SIGNAL_LAST];

void gclue_hybris_binder_dropGnss (GClueHybrisBinder *hbinder);

gboolean gclue_hybris_binder_gnssInit (GClueHybris *hybris);
gboolean gclue_hybris_binder_gnssStart (GClueHybris *hybris);
gboolean gclue_hybris_binder_gnssStop (GClueHybris *hybris);
void gclue_hybris_binder_gnssCleanup (GClueHybris *hybris);
gboolean gclue_hybris_binder_gnssInjectTime (GClueHybris *hybris,
                                             HybrisGnssUtcTime timeMs,
                                             int64_t timeReferenceMs,
                                             int32_t uncertaintyMs);
gboolean gclue_hybris_binder_gnssInjectLocation (GClueHybris *hybris,
                                                 double latitudeDegrees,
                                                 double longitudeDegrees,
                                                 float accuracyMeters);
void gclue_hybris_binder_gnssDeleteAidingData (GClueHybris *hybris,
                                               HybrisGnssAidingData aidingDataFlags);
gboolean gclue_hybris_binder_gnssSetPositionMode (GClueHybris *hybris,
                                                  HybrisGnssPositionMode mode,
                                                  HybrisGnssPositionRecurrence recurrence,
                                                  guint32 minIntervalMs,
                                                  guint32 preferredAccuracyMeters,
                                                  guint32 preferredTimeMs);

void gclue_hybris_binder_gnssDebugInit (GClueHybris *hybris);

void gclue_hybris_binder_gnssNiInit (GClueHybris *hybris);
void gclue_hybris_binder_gnssNiRespond (GClueHybris *hybris,
                                        int32_t notifId,
                                        HybrisGnssUserResponseType userResponse);

void gclue_hybris_binder_gnssXtraInit (GClueHybris *hybris);
gboolean gclue_hybris_binder_gnssXtraInjectXtraData (GClueHybris *hybris,
                                                     gchar *xtraData);

void gclue_hybris_binder_aGnssInit (GClueHybris *hybris);
gboolean gclue_hybris_binder_aGnssDataConnClosed (GClueHybris *hybris);
gboolean gclue_hybris_binder_aGnssDataConnFailed (GClueHybris *hybris);
gboolean gclue_hybris_binder_aGnssDataConnOpen (GClueHybris *hybris,
                                                const char* apn,
                                                const char* protocol);
gboolean gclue_hybris_binder_aGnssSetServer (GClueHybris *hybris,
                                             HybrisAGnssType type,
                                             const char *hostname,
                                             int port);

gboolean gclue_hybris_binder_aGnssRilsetSetId (GClueHybris *hybris,
                                               int type,
                                               const char *setid);
void gclue_hybris_binder_aGnssRilInit (GClueHybris *hybris);

enum GnssFunctions {
    GNSS_SET_CALLBACK = 1,
    GNSS_START = 2,
    GNSS_STOP = 3,
    GNSS_CLEANUP = 4,
    GNSS_INJECT_TIME = 5,
    GNSS_INJECT_LOCATION = 6,
    GNSS_DELETE_AIDING_DATA = 7,
    GNSS_SET_POSITION_MODE = 8,
    GNSS_GET_EXTENSION_AGNSS_RIL = 9,
    GNSS_GET_EXTENSION_GNSS_GEOFENCING = 10,
    GNSS_GET_EXTENSION_AGNSS = 11,
    GNSS_GET_EXTENSION_GNSS_NI = 12,
    GNSS_GET_EXTENSION_GNSS_MEASUREMENT = 13,
    GNSS_GET_EXTENSION_GNSS_NAVIGATION_MESSAGE = 14,
    GNSS_GET_EXTENSION_XTRA = 15,
    GNSS_GET_EXTENSION_GNSS_CONFIGURATION = 16,
    GNSS_GET_EXTENSION_GNSS_DEBUG = 17,
    GNSS_GET_EXTENSION_GNSS_BATCHING = 18
};

enum GnssCallbacks {
    GNSS_LOCATION_CB = 1,
    GNSS_STATUS_CB = 2,
    GNSS_SV_STATUS_CB = 3,
    GNSS_NMEA_CB = 4,
    GNSS_SET_CAPABILITIES_CB = 5,
    GNSS_ACQUIRE_WAKELOCK_CB = 6,
    GNSS_RELEASE_WAKELOCK_CB = 7,
    GNSS_REQUEST_TIME_CB = 8,
    GNSS_SET_SYSTEM_INFO_CB = 9
};

enum GnssDebugFunctions {
    GNSS_DEBUG_GET_DEBUG_DATA = 1
};

enum GnssNiFunctions {
    GNSS_NI_SET_CALLBACK = 1,
    GNSS_NI_RESPOND = 2
};

enum GnssNiCallbacks {
    GNSS_NI_NOTIFY_CB = 1
};

enum GnssXtraFunctions {
    GNSS_XTRA_SET_CALLBACK = 1,
    GNSS_XTRA_INJECT_XTRA_DATA = 2
};

enum GnssXtraCallbacks {
    GNSS_XTRA_DOWNLOAD_REQUEST_CB = 1
};

enum AGnssFunctions {
    AGNSS_SET_CALLBACK = 1,
    AGNSS_DATA_CONN_CLOSED = 2,
    AGNSS_DATA_CONN_FAILED = 3,
    AGNSS_SET_SERVER = 4,
    AGNSS_DATA_CONN_OPEN = 5
};

enum AGnssCallbacks {
    AGNSS_STATUS_IP_V4_CB = 1,
    AGNSS_STATUS_IP_V6_CB = 2,
    AGNSS_STATUS_CB = 3
};

enum AGnssRilFunctions {
    AGNSS_RIL_SET_CALLBACK = 1,
    AGNSS_RIL_SET_REF_LOCATION = 2,
    AGNSS_RIL_SET_SET_ID = 3,
    AGNSS_RIL_UPDATE_NETWORK_STATE = 4,
    AGNSS_RIL_UPDATE_NETWORK_AVAILABILITY = 5
};

enum AGnssRilCallbacks {
    AGNSS_RIL_REQUEST_SET_ID_CB = 1,
    AGNSS_RIL_REQUEST_REF_LOC_CB = 2
};

enum HybrisApnIpTypeEnum {
    HYBRIS_APN_IP_INVALID  = 0,
    HYBRIS_APN_IP_IPV4     = 1,
    HYBRIS_APN_IP_IPV6     = 2,
    HYBRIS_APN_IP_IPV4V6   = 3
};

enum GnssFunctions_1_1 {
    GNSS_SET_CALLBACK_1_1 = 19,
    GNNS_SET_POSITION_MODE_1_1 = 20,
    GNSS_GET_EXTENSION_GNSS_CONFIGURATION_1_1 = 21,
    GNSS_GET_EXTENSION_GNSS_MEASUREMENT_1_1 = 22,
    GNSS_INJECT_BEST_LOCATION = 23
};

enum GnssCallbacks_1_1 {
    GNSS_NAME_CB = 10,
    GNSS_REQUEST_LOCATION_CB = 11
};

enum GnssFunctions_2_0 {
    GNSS_SET_CALLBACK_2_0 = 24,
    GNSS_GET_EXTENSION_GNSS_CONFIGURATION_2_0 = 25,
    GNSS_GET_EXTENSION_GNSS_DEBUG_2_0 = 26,
    GNSS_GET_EXTENSION_AGNSS_2_0 = 27,
    GNSS_GET_EXTENSION_AGNSS_RIL_2_0 = 28,
    GNSS_GET_EXTENSION_GNSS_MEASUREMENTS_2_0 = 29,
    GNSS_GET_EXTENSION_GNSS_MEASUREMENT_CORRECTIONS_2_0 = 29,
    GNSS_GET_EXTENSION_VISIBILITY_CONTROL = 30,
    GNSS_GET_EXTENSION_GNSS_BATCHING_2_0 = 31,
    GNSS_INJECT_BEST_LOCATION_2_0 = 32
};

enum GnssCallbacks_2_0 {
    GNSS_SET_CAPABILITIES_CB_2_0 = 12,
    GNSS_LOCATION_CB_2_0 = 13,
    GNSS_REQUEST_LOCATION_CB_2_0 = 14,
    GNSS_SV_STATUS_CB_2_0 = 15
};

#define GNSS_IFACE(x)       "android.hardware.gnss@1.0::" x
#define GNSS_REMOTE         GNSS_IFACE ("IGnss")
#define GNSS_CALLBACK       GNSS_IFACE ("IGnssCallback")
#define GNSS_DEBUG_REMOTE   GNSS_IFACE ("IGnssDebug")
#define GNSS_NI_REMOTE      GNSS_IFACE ("IGnssNi")
#define GNSS_NI_CALLBACK    GNSS_IFACE ("IGnssNiCallback")
#define GNSS_XTRA_REMOTE    GNSS_IFACE ("IGnssXtra")
#define GNSS_XTRA_CALLBACK  GNSS_IFACE ("IGnssXtraCallback")
#define AGNSS_REMOTE        GNSS_IFACE ("IAGnss")
#define AGNSS_CALLBACK      GNSS_IFACE ("IAGnssCallback")
#define AGNSS_RIL_REMOTE    GNSS_IFACE ("IAGnssRil")
#define AGNSS_RIL_CALLBACK  GNSS_IFACE ("IAGnssRilCallback")

#define GNSS_IFACE_2_0(x)       "android.hardware.gnss@2.0::" x
#define GNSS_REMOTE_2_0         GNSS_IFACE_2_0 ("IGnss")
#define GNSS_CALLBACK_2_0       GNSS_IFACE_2_0 ("IGnssCallback")
#define AGNSS_REMOTE_2_0        GNSS_IFACE_2_0 ("IAGnss")
#define AGNSS_CALLBACK_2_0      GNSS_IFACE_2_0 ("IAGnssCallback")
#define AGNSS_RIL_REMOTE_2_0    GNSS_IFACE_2_0 ("IAGnssRil")
#define AGNSS_RIL_CALLBACK_2_0  GNSS_IFACE_2_0 ("IAGnssRilCallback")

#define NTP_TIMESTAMP_DELTA 2208988800ull

typedef struct {
    uint16_t seconds;
    uint16_t fraction;
} NtpShort;

typedef struct {
    uint32_t seconds;
    uint32_t fraction;
} NtpTime;

typedef struct {
    uint8_t flags;
    uint8_t stratum;
    int8_t poll;
    int8_t precision;
    NtpShort rootDelay;
    NtpShort rootDispersion;
    uint32_t referenceId;
    NtpTime referenceTimestamp;
    NtpTime originTimestamp;
    NtpTime receiveTimestamp;
    NtpTime transmitTimestamp;
} NtpMessage;

/*==========================================================================*
 * Implementation
 *==========================================================================*/

int
get_subscriber_identity (char** imsi)
{
    DBusError error;
    dbus_error_init (&error);
    DBusConnection *conn;

    conn = dbus_bus_get (DBUS_BUS_SYSTEM, &error);
    if (dbus_error_is_set (&error)) {
        dbus_error_free (&error);
        return 0;
    }

    DBusMessage *msg, *reply;
    DBusMessageIter args, array_iter, dict_iter;
    int found = 0;

    msg = dbus_message_new_method_call ("org.ofono",
                                        "/ril_0",
                                        "org.ofono.SimManager",
                                        "GetProperties");
    if (msg == NULL) {
        msg = dbus_message_new_method_call ("org.ofono",
                                            "/ril_1",
                                            "org.ofono.SimManager",
                                            "GetProperties");

        if (msg == NULL)
            return 0;
    }

    reply = dbus_connection_send_with_reply_and_block (conn, msg, -1, &error);
    if (dbus_error_is_set (&error)) {
        dbus_error_free (&error);
        dbus_message_unref (msg);
        return 0;
    }

    if (dbus_message_iter_init (reply, &args) &&
        DBUS_TYPE_ARRAY == dbus_message_iter_get_arg_type (&args)) {
        dbus_message_iter_recurse (&args, &array_iter);

        while (dbus_message_iter_get_arg_type (&array_iter) != DBUS_TYPE_INVALID) {
            dbus_message_iter_recurse (&array_iter, &dict_iter);
            DBusMessageIter sub_iter;
            char *prop_name;
            char *prop_value;

            dbus_message_iter_get_basic (&dict_iter, &prop_name);

            if (strcmp (prop_name, "SubscriberIdentity") == 0) {
                dbus_message_iter_next (&dict_iter);
                dbus_message_iter_recurse (&dict_iter, &sub_iter);
                dbus_message_iter_get_basic (&sub_iter, &prop_value);
                *imsi = strdup (prop_value);
                found = 1;
            }
            dbus_message_iter_next (&array_iter);
        }
    } else
        return 0;

    dbus_message_unref (reply);
    dbus_message_unref (msg);
    return found;
}

bool
service_exists (GBinderServiceManager* sm,
                const char* service)
{
    GBinderRemoteObject *obj = gbinder_servicemanager_get_service_sync (sm, service, NULL);

    if (obj)
        return true;
    else
        return false;

    gbinder_remote_object_unref (obj);
    obj = NULL;
}

HybrisApnIpType
protocol_to_apn_type (const char* protocol)
{
    if (strcmp (protocol, "ip") == 0)
        return HYBRIS_APN_IP_IPV4;
    else if (strcmp (protocol, "ipv6") == 0)
        return HYBRIS_APN_IP_IPV6;
    else if (strcmp (protocol, "dual") == 0)
        return HYBRIS_APN_IP_IPV4V6;
    else
        return HYBRIS_APN_IP_INVALID;
}

void
print_capabilities (guint32 capabilities)
{
    g_debug ("lcGeoclueHybris: capabilities:");
    if (capabilities & SCHEDULING)
        g_debug ("  - SCHEDULING");
    if (capabilities & MSB)
        g_debug ("  - MSB");
    if (capabilities & MSA)
        g_debug ("  - MSA");
    if (capabilities & SINGLE_SHOT)
        g_debug ("  - SINGLE_SHOT");
    if (capabilities & ON_DEMAND_TIME)
        g_debug ("  - ON_DEMAND_TIME");
    if (capabilities & GEOFENCING)
        g_debug ("  - GEOFENCING");
    if (capabilities & MEASUREMENTS)
        g_debug ("  - MEASUREMENTS");
    if (capabilities & NAV_MESSAGES)
        g_debug ("  - NAV_MESSAGES");
}

gboolean
parse_supl_string (const char *supl,
                   char **domain,
                   int *port)
{
    if (!supl)
        return FALSE;

    if (supl[0] == '"' && supl[strlen (supl) - 1] == '"') {
        char *cleaned_supl = strdup (supl + 1);
        cleaned_supl[strlen (cleaned_supl) - 1] = '\0';
        supl = cleaned_supl;
    } else
        supl = strdup (supl);

    char *colon = strchr(supl, ':');
    if (!colon)
        return FALSE;

    if (colon == supl)
        return FALSE;

    if (*(colon + 1) == '\0')
        return FALSE;

    *domain = g_strndup(supl, colon - supl);
    if (!*domain)
        return FALSE;

    *port = atoi(colon + 1);
    if (*port == 0 && strcmp(colon + 1, "0") != 0) {
        g_free(*domain);
        *domain = NULL;
        return FALSE;
    }

    return TRUE;
}

int
query_ntp_server (int64_t *timeMs,
                  int *uncertaintyMs,
                  int64_t *timeReferenceMs,
                  const char *ntpserver)
{
    if (ntpserver[0] == '"' && ntpserver[strlen (ntpserver) - 1] == '"') {
        char *cleaned_ntpserver = strdup (ntpserver + 1);
        cleaned_ntpserver[strlen (cleaned_ntpserver) - 1] = '\0';
        ntpserver = cleaned_ntpserver;
    } else
        ntpserver = strdup (ntpserver);

    int sockfd = socket (AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        return 0;

    struct timeval timeout;
    // timeout after 3 seconds
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*) &timeout, sizeof (timeout)) < 0) {
        close (sockfd);
        return 0;
    }

    struct hostent *server = gethostbyname (ntpserver);
    if (server == NULL) {
        close (sockfd);
        return 0;
    }

    struct sockaddr_in servaddr;
    memset (&servaddr, 0, sizeof (servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons (123);
    memcpy (&servaddr.sin_addr.s_addr, server->h_addr, server->h_length);

    NtpMessage request;
    memset (&request, 0, sizeof (NtpMessage));
    request.flags = 0x1B;

    struct timeval currentTime;
    gettimeofday (&currentTime, NULL);
    request.transmitTimestamp.seconds = htonl (currentTime.tv_sec + NTP_TIMESTAMP_DELTA);
    request.transmitTimestamp.fraction = htonl ((uint32_t) ((double) (currentTime.tv_usec + 1) * (double) (1LL << 32) / 1000000));
    int64_t requestTicks = currentTime.tv_sec * 1000 + currentTime.tv_usec / 1000;

    sendto (sockfd, (const char *) &request, sizeof (NtpMessage), 0,
           (const struct sockaddr *) &servaddr, sizeof (servaddr));

    NtpMessage response;
    socklen_t len = sizeof (servaddr);
    int n = recvfrom (sockfd, (char *) &response, sizeof (NtpMessage),
                     MSG_WAITALL, (struct sockaddr *) &servaddr, &len);

    struct timeval receiveTime;
    gettimeofday (&receiveTime, NULL);
    int64_t responseTicks = receiveTime.tv_sec * 1000 + receiveTime.tv_usec / 1000;

    if (n < 0) {
        close (sockfd);
        return 0;
    }

    int64_t secs = (int64_t) ntohl (response.transmitTimestamp.seconds) - NTP_TIMESTAMP_DELTA;
    *timeMs = secs * 1000 + ((int64_t) ntohl (response.transmitTimestamp.fraction) * 1000) / (1LL << 32);
    *timeReferenceMs = responseTicks;
    *uncertaintyMs = abs((int) ((responseTicks - requestTicks) / 2));

    close(sockfd);
    return 1;
}

static const char *
get_ntp_server_config ()
{
        GClueConfig *config;

        config = gclue_config_get_singleton ();
        return gclue_config_get_hybris_ntp_server (config);
}

static const char *
get_supl_server_config ()
{
        GClueConfig *config;

        config = gclue_config_get_singleton ();
        if (!gclue_config_get_hybris_supl_enabled (config))
                return NULL;

        return gclue_config_get_hybris_supl_server (config);
}

const void *
geoclue_binder_gnss_decode_struct1(GBinderReader *in,
                                   guint size)
{
    const void *result = NULL;
    GBinderBuffer *buf = gbinder_reader_read_buffer (in);

    if (buf && buf->size == size)
        result = buf->data;

    gbinder_buffer_free (buf);
    return result;
}

#define geoclue_binder_gnss_decode_struct(type,in) \
    ((const type*)geoclue_binder_gnss_decode_struct1(in, sizeof(type)))

gboolean
nmea_checksum_valid (GString *nmea)
{
    unsigned char checksum = 0;
    for (int i = 1; i < nmea->len; ++i) {
        if (nmea->str[i] == '*') {
            if (nmea->len < i+3)
                return FALSE;

            checksum ^= g_ascii_strtoll (g_string_new_len (nmea->str + i + 1, 2)->str, NULL, 16);

            break;
        }

        checksum ^= nmea->str[i];
    }

    return checksum == 0;
}

void
parse_rmc (GString *nmea)
{
    gchar **fields = g_strsplit (nmea->str, ",", 0);
    if (g_strv_length (fields) < 12)
        return;

    if (fields[10]) {
        double variation = g_strtod (fields[10], NULL);
        if (fields[11][0] == 'W')
            variation = -variation;
    }
}

void
process_nmea (gint64 timestamp,
              const char *nmea_data)
{
    GString *nmea;
    int length = strlen (nmea_data);
    while (length > 0 && g_ascii_isspace (nmea_data[length - 1]))
        --length;

    if (length == 0)
        return;

    nmea = g_string_new_len (nmea_data, length);

    g_debug("lcGeoclueHybrisNmea: timestamp %s", nmea_data);

    if (!nmea_checksum_valid (nmea))
        return;

    // truncate checksum and * from end of sentence
    nmea = g_string_truncate (nmea, nmea->len);

    if (g_str_has_prefix (nmea->str, "$GPRMC"))
        parse_rmc (nmea);
}

const double MpsToKnots = 1.943844;

GBinderLocalReply *
geoclue_binder_gnss_callback(GBinderLocalObject *obj,
                             GBinderRemoteRequest *req,
                             guint code,
                             guint flags,
                             int *status,
                             void *user_data)
{
    const char *iface = gbinder_remote_request_interface (req);
    GClueHybrisBinder *hbinder = (GClueHybrisBinder *) user_data;

    if (!g_strcmp0 (iface, GNSS_CALLBACK)) {
        GBinderReader reader;

        gbinder_remote_request_init_reader (req, &reader);
        switch (code) {
        case GNSS_LOCATION_CB:
            {
            g_debug ("lcGeoclueHybrisGnss: GNSS location");
            GClueHybrisLocation* loc = g_slice_new0 (GClueHybrisLocation);

            const GnssLocation *location = geoclue_binder_gnss_decode_struct (GnssLocation, &reader);

            loc->timestamp = location->timestamp;
            g_debug ("lcGeoclueHybrisGnss: Location timestamp: %" G_GUINT64_FORMAT, loc->timestamp);

            if (location->gnssLocationFlags & HYBRIS_GNSS_LOCATION_HAS_LAT_LONG) {
                loc->latitude = location->latitudeDegrees;
                loc->longitude = location->longitudeDegrees;
                g_debug ("lcGeoclueHybrisGnss: Latitude: %f, Longitude: %f", loc->latitude, loc->longitude);
            }

            if (location->gnssLocationFlags & HYBRIS_GNSS_LOCATION_HAS_ALTITUDE) {
                loc->altitude = location->altitudeMeters;
                g_debug ("lcGeoclueHybrisGnss: Altitude: %f meters", loc->altitude);
            }

            if (location->gnssLocationFlags & HYBRIS_GNSS_LOCATION_HAS_SPEED) {
                loc->speed = location->speedMetersPerSec * MpsToKnots;
                g_debug ("lcGeoclueHybrisGnss: Speed: %f knots", loc->speed);
            }

            if (location->gnssLocationFlags & HYBRIS_GNSS_LOCATION_HAS_BEARING) {
                loc->direction = location->bearingDegrees;
                g_debug ("lcGeoclueHybrisGnss: Bearing: %f degrees", loc->direction);
            }

            if ((location->gnssLocationFlags & HYBRIS_GNSS_LOCATION_HAS_HORIZONTAL_ACCURACY) ||
                (location->gnssLocationFlags & HYBRIS_GNSS_LOCATION_HAS_VERTICAL_ACCURACY)) {
                GClueHybrisAccuracy* accuracy = g_slice_new0 (GClueHybrisAccuracy);
                if (location->gnssLocationFlags & HYBRIS_GNSS_LOCATION_HAS_HORIZONTAL_ACCURACY) {
                    accuracy->horizontal = location->horizontalAccuracyMeters;
                    g_debug ("lcGeoclueHybrisGnss: Horizontal Accuracy: %f meters", accuracy->horizontal);
                }
                if (location->gnssLocationFlags & HYBRIS_GNSS_LOCATION_HAS_VERTICAL_ACCURACY) {
                    accuracy->vertical = location->verticalAccuracyMeters;
                    g_debug ("lcGeoclueHybrisGnss: Vertical Accuracy: %f meters", accuracy->vertical);
                }
                loc->accuracy = accuracy;
            }

            g_signal_emit (hbinder, signals[SET_LOCATION], 0, loc);
            }
            break;
        case GNSS_STATUS_CB:
            {
            guint32 stat;
            if (gbinder_reader_read_uint32 (&reader, &stat)) {
                if (stat == HYBRIS_GNSS_STATUS_ENGINE_ON)
                    g_debug ("lcGeoclueHybris: GNSS status engine on");

                if (stat == HYBRIS_GNSS_STATUS_ENGINE_OFF)
                    g_debug ("lcGeoclueHybris: GNSS status engine off");

                if (stat == HYBRIS_GNSS_STATUS_SESSION_END)
                    g_debug ("lcGeoclueHybris: GNSS status session end");

                if (stat == HYBRIS_GNSS_STATUS_SESSION_BEGIN)
                    g_debug("lcGeoclueHybris: GNSS status session begin");
            }
            }
            break;
        case GNSS_SV_STATUS_CB:
            {
            const GnssSvStatus *sv_status = geoclue_binder_gnss_decode_struct (GnssSvStatus, &reader);

            g_debug ("lcGeoclueHybrisSv: Number of SVs: %u", sv_status->numSvs);

            GList *satellites = g_list_alloc ();
            GList *used_prns = g_list_alloc ();

            for (int i = 0; i < sv_status->numSvs; ++i) {
                GClueHybrisSatelliteInfo *sat_info = g_slice_new0 (GClueHybrisSatelliteInfo);
                GnssSvInfo sv_info = sv_status->gnssSvList[i];

                g_debug ("lcGeoclueHybrisSv: SV ID: %d, Constellation: %d, C/N0 dB-Hz: %f, Elevation: %f, Azimuth: %f, Carrier frequency: %f",
                         sv_info.svid, sv_info.constellation, sv_info.cN0Dbhz, sv_info.elevationDegrees, sv_info.azimuthDegrees, sv_info.carrierFrequencyHz);

                sat_info->snr = sv_info.cN0Dbhz;
                sat_info->elevation = sv_info.elevationDegrees;
                sat_info->azimuth = sv_info.azimuthDegrees;
                int prn = sv_info.svid;
                // From https://github.com/barbeau/gpstest
                // and https://github.com/mvglasow/satstat/wiki/NMEA-IDs
                if (sv_info.constellation == SBAS) {
                    g_debug ("lcGeoclueHybrisSv: SV constellation is SBAS");
                    prn -= 87;
                } else if (sv_info.constellation == GLONASS) {
                    g_debug ("lcGeoclueHybrisSv: SV constellation is GLONASS");
                    prn += 64;
                } else if (sv_info.constellation == BEIDOU) {
                    g_debug ("lcGeoclueHybrisSv: SV constellation is BEIDOU");
                    prn += 200;
                } else if (sv_info.constellation == GALILEO) {
                    g_debug ("lcGeoclueHybrisSv: SV constellation is GALILEO");
                    prn += 300;
                } else if (sv_info.constellation == GPS)
                    g_debug ("lcGeoclueHybrisSv: SV constellation is GPS");
                else if (sv_info.constellation == QZSS)
                    g_debug ("lcGeoclueHybrisSv: SV constellation is QZSS");

                sat_info->prn = prn;
                satellites = g_list_append (satellites, sat_info);

                g_debug ("lcGeoclueHybrisSv: Adjusted PRN: %d", prn);

                if (sv_info.svFlag & HYBRIS_GNSS_SV_FLAGS_USED_IN_FIX) {
                    used_prns = g_list_append (used_prns, &prn);
                    g_debug ("lcGeoclueHybrisSv: SV %d used in fix", sv_info.svid);
                } else if (sv_info.svFlag & HYBRIS_GNSS_SV_FLAGS_NONE)
                    g_debug("lcGeoclueHybrisSv: SV %d has no flags", sv_info.svid);
                else if (sv_info.svFlag & HYBRIS_GNSS_SV_FLAGS_HAS_EPHEMERIS_DATA)
                    g_debug("lcGeoclueHybrisSv: SV %d has ephemeris data", sv_info.svid);
                else if (sv_info.svFlag & HYBRIS_GNSS_SV_FLAGS_HAS_ALMANAC_DATA)
                    g_debug("lcGeoclueHybrisSv: SV %d has almanac data", sv_info.svid);
                else if (sv_info.svFlag & HYBRIS_GNSS_SV_FLAGS_HAS_CARRIER_FREQUENCY)
                    g_debug("lcGeoclueHybrisSv: SV %d has carrier frequency", sv_info.svid);
            }
            }
            break;
        case GNSS_NMEA_CB:
            {
            gint64 timestamp;
            if (gbinder_reader_read_int64 (&reader, &timestamp)) {
                char *nmea_data = gbinder_reader_read_hidl_string (&reader);
                if (nmea_data) {
                    process_nmea (timestamp, nmea_data);
                    g_free (nmea_data);
                }
            }
            }
            break;
        case GNSS_SET_CAPABILITIES_CB:
            {
            guint32 capabilities;
            if (gbinder_reader_read_uint32 (&reader, &capabilities))
                print_capabilities (capabilities);
            }
            break;
        case GNSS_ACQUIRE_WAKELOCK_CB:
        case GNSS_RELEASE_WAKELOCK_CB:
            break;
        case GNSS_REQUEST_TIME_CB:
            g_debug ("lcGeoclueHybris: GNSS request UTC time");
            break;
        case GNSS_SET_SYSTEM_INFO_CB:
            {
            guint16 year_of_hw;
            if (gbinder_reader_read_uint16 (&reader, &year_of_hw))
                g_debug ("lcGeoclueHybris: GNSS set system info year %d", year_of_hw);
            }
            break;
        default:
            g_warning ("Failed to decode GNSS callback %u", code);
            break;
        }
        *status = GBINDER_STATUS_OK;
        return gbinder_local_reply_append_int32 (gbinder_local_object_new_reply (obj), 0);
    } else {
        g_warning ("Unknown interface %s and code %u", iface, code);
        *status = GBINDER_STATUS_FAILED;
    }
    return NULL;
}

GBinderLocalReply *
geoclue_binder_gnss_xtra_callback (GBinderLocalObject *obj,
                                   GBinderRemoteRequest *req,
                                   guint code,
                                   guint flags,
                                   int *status,
                                   void *user_data)
{
    const char *iface = gbinder_remote_request_interface (req);

    if (!g_strcmp0 (iface, GNSS_XTRA_CALLBACK)) {
        GBinderReader reader;

        gbinder_remote_request_init_reader (req, &reader);
        switch (code) {
        case GNSS_XTRA_DOWNLOAD_REQUEST_CB:
            g_debug ("lcGeoclueHybris: XTRA download request");
            break;
        default:
            g_warning ("Failed to decode GNSS XTRA callback %u", code);
            break;
        }
        *status = GBINDER_STATUS_OK;
        return gbinder_local_reply_append_int32 (gbinder_local_object_new_reply (obj), 0);
    } else {
        g_warning ("Unknown interface %s and code %u", iface, code);
        *status = GBINDER_STATUS_FAILED;
    }
    return NULL;
}

GBinderLocalReply *
geoclue_binder_agnss_callback (GBinderLocalObject *obj,
                               GBinderRemoteRequest *req,
                               guint code,
                               guint flags,
                               int *status,
                               void *user_data)
{
    const char *iface = gbinder_remote_request_interface (req);
    if (!g_strcmp0 (iface, AGNSS_CALLBACK)) {
        GBinderReader reader;

        gbinder_remote_request_init_reader (req, &reader);
        switch (code) {
        case AGNSS_STATUS_IP_V4_CB:
            {
            gint32 ipv4;

            const AGnssStatusIpV4 *status = geoclue_binder_gnss_decode_struct (AGnssStatusIpV4, &reader);

            ipv4 = status->ipV4Addr;
            g_debug ("lcGeoclueHybris: AGNSS IPv4 %d", ipv4);

            if (status->type & TYPE_SUPL)
                g_debug ("lcGeoclueHybris: AGNSS type is SUPL");

            if (status->type & TYPE_C2K)
                g_debug ("lcGeoclueHybris: AGNSS type is C2K");

            if (status->type & TYPE_SUPL_EIMS)
                g_debug ("lcGeoclueHybris: AGNSS type is SUPL EIMS");

            if (status->type & TYPE_SUPL_IMS)
                g_debug ("lcGeoclueHybris: AGNSS type is SUPL IMS");

            if (status->status & REQUEST_AGNSS_DATA_CONN)
                g_debug ("lcGeoclueHybris: AGNSS request data conn");

            if (status->status & RELEASE_AGNSS_DATA_CONN)
                g_debug ("lcGeoclueHybris: AGNSS release data conn");

            if (status->status & AGNSS_STATUS_DATA_CONNECTED)
                g_debug ("lcGeoclueHybris: AGNSS data connected");

            if (status->status & AGNSS_STATUS_DATA_CONN_DONE)
                g_debug ("lcGeoclueHybris: AGNSS data conn done");

            if (status->status & AGNSS_STATUS_DATA_CONN_FAILED)
                g_debug ("lcGeoclueHybris: AGNSS data conn failed");

            }
            break;
        case AGNSS_STATUS_IP_V6_CB:
            {
            const guint8* ipv6;

            const AGnssStatusIpV6 *status = geoclue_binder_gnss_decode_struct (AGnssStatusIpV6, &reader);

            ipv6 = status->ipV6Addr;
            g_debug ("lcGeoclueHybris: AGNSS IPv6");

            if (status->type & TYPE_SUPL)
                g_debug ("lcGeoclueHybris: AGNSS type is SUPL");

            if (status->type & TYPE_C2K)
                g_debug ("lcGeoclueHybris: AGNSS type is C2K");

            if (status->type & TYPE_SUPL_EIMS)
                g_debug ("lcGeoclueHybris: AGNSS type is SUPL EIMS");

            if (status->type & TYPE_SUPL_IMS)
                g_debug ("lcGeoclueHybris: AGNSS type is SUPL IMS");

            if (status->status & REQUEST_AGNSS_DATA_CONN)
                g_debug ("lcGeoclueHybris: AGNSS request data conn");

            if (status->status & RELEASE_AGNSS_DATA_CONN)
                g_debug ("lcGeoclueHybris: AGNSS release data conn");

            if (status->status & AGNSS_STATUS_DATA_CONNECTED)
                g_debug ("lcGeoclueHybris: AGNSS data connected");

            if (status->status & AGNSS_STATUS_DATA_CONN_DONE)
                g_debug ("lcGeoclueHybris: AGNSS data conn done");

            if (status->status & AGNSS_STATUS_DATA_CONN_FAILED)
                g_debug ("lcGeoclueHybris: AGNSS data conn failed");

            }
            break;
        default:
            g_warning ("Failed to decode AGNSS callback %u", code);
            break;
        }
        *status = GBINDER_STATUS_OK;
        return gbinder_local_reply_append_int32 (gbinder_local_object_new_reply (obj), 0);
    } else {
        g_warning ("Unknown interface %s and code %u", iface, code);
        *status = GBINDER_STATUS_FAILED;
    }
    return NULL;
}


GBinderLocalReply *
geoclue_binder_agnss_ril_callback (GBinderLocalObject *obj,
                                   GBinderRemoteRequest *req,
                                   guint code,
                                   guint flags,
                                   int *status,
                                   void *user_data)
{
    const char *iface = gbinder_remote_request_interface (req);

    if (!g_strcmp0 (iface, AGNSS_RIL_CALLBACK)) {
        GBinderReader reader;

        gbinder_remote_request_init_reader (req, &reader);
        switch (code) {
        case AGNSS_RIL_REQUEST_SET_ID_CB:
            {
            guint32 id;
            if (gbinder_reader_read_uint32 (&reader, &id)) {
                switch (id) {
                    case IMSI:
                        g_debug ("lcGeoclueHybris: AGNSS RIL request set ID IMSI");
                        break;
                    case MSISDN:
                        g_debug ("lcGeoclueHybris: AGNSS RIL request set ID MSISDN");
                        break;
                    default:
                        g_debug ("lcGeoclueHybris: AGNSS RIL request set unknown ID %d", id);
                        break;
                }
            }
            }
            break;
        case AGNSS_RIL_REQUEST_REF_LOC_CB:
            g_debug ("lcGeoclueHybris: AGNSS RIL request ref location");
            break;
        default:
            g_warning ("Failed to decode AGNSS RIL callback %u", code);
            break;
        }
        *status = GBINDER_STATUS_OK;
        return gbinder_local_reply_append_int32 (gbinder_local_object_new_reply(obj), 0);
    } else {
        g_warning ("Unknown interface %s and code %u", iface, code);
        *status = GBINDER_STATUS_FAILED;
    }
    return NULL;
}


GBinderLocalReply *
geoclue_binder_gnss_ni_callback (GBinderLocalObject *obj,
                                 GBinderRemoteRequest *req,
                                 guint code,
                                 guint flags,
                                 int *status,
                                 void *user_data)
{
    const char *iface = gbinder_remote_request_interface (req);

    if (!g_strcmp0 (iface, GNSS_NI_CALLBACK)) {
        GBinderReader reader;

        gbinder_remote_request_init_reader (req, &reader);
        switch (code) {
        case GNSS_NI_NOTIFY_CB:
            g_debug ("lcGeoclueHybris: GNSS NI notify");
            break;
        default:
            g_warning ("Failed to decode GNSS NI callback %u", code);
            break;
        }
        *status = GBINDER_STATUS_OK;
        return gbinder_local_reply_append_int32 (gbinder_local_object_new_reply (obj), 0);
    } else {
        g_warning ("Unknown interface %s and code %u", iface, code);
        *status = GBINDER_STATUS_FAILED;
    }
    return NULL;
}

void
geoclue_binder_gnss_gnss_died (GBinderRemoteObject *obj,
                               void *user_data)
{
    GClueHybrisBinder *hbinder = (GClueHybrisBinder *) user_data;
    gclue_hybris_binder_dropGnss (hbinder);
}

/*==========================================================================*
 * Backend class
 *==========================================================================*/

static void
gclue_hybris_binder_class_init (GClueHybrisBinderClass *klass)
{
    signals[SET_LOCATION] = g_signal_lookup ("setLocation", GCLUE_TYPE_HYBRIS);
}

static void
gclue_hybris_interface_init (GClueHybrisInterface *iface)
{
    iface->gnssInit = gclue_hybris_binder_gnssInit;
    iface->gnssStart = gclue_hybris_binder_gnssStart;
    iface->gnssStop = gclue_hybris_binder_gnssStop;
    iface->gnssCleanup = gclue_hybris_binder_gnssCleanup;
    iface->gnssInjectTime = gclue_hybris_binder_gnssInjectTime;
    iface->gnssInjectLocation = gclue_hybris_binder_gnssInjectLocation;
    iface->gnssDeleteAidingData = gclue_hybris_binder_gnssDeleteAidingData;
    iface->gnssSetPositionMode = gclue_hybris_binder_gnssSetPositionMode;
    iface->gnssDebugInit = gclue_hybris_binder_gnssDebugInit;
    iface->gnssNiInit = gclue_hybris_binder_gnssNiInit;
    iface->gnssNiRespond = gclue_hybris_binder_gnssNiRespond;
    iface->gnssXtraInit = gclue_hybris_binder_gnssXtraInit;
    iface->gnssXtraInjectXtraData = gclue_hybris_binder_gnssXtraInjectXtraData;
    iface->aGnssInit = gclue_hybris_binder_aGnssInit;
    iface->aGnssDataConnClosed = gclue_hybris_binder_aGnssDataConnClosed;
    iface->aGnssDataConnFailed = gclue_hybris_binder_aGnssDataConnFailed;
    iface->aGnssDataConnOpen = gclue_hybris_binder_aGnssDataConnOpen;
    iface->aGnssSetServer = gclue_hybris_binder_aGnssSetServer;
    iface->aGnssRilInit = gclue_hybris_binder_aGnssRilInit;
    iface->aGnssRilsetSetId = gclue_hybris_binder_aGnssRilsetSetId;
}

void
gclue_hybris_binder_dropGnss (GClueHybrisBinder *hbinder)
{
    GClueHybrisBinderPrivate *priv = hbinder->priv;

    if (priv->m_callbackGnss) {
        gbinder_local_object_drop (priv->m_callbackGnss);
        priv->m_callbackGnss = NULL;
    }
    if (priv->m_clientGnss) {
        gbinder_client_unref (priv->m_clientGnss);
        priv->m_clientGnss = NULL;

        if (priv->m_clientGnss_2_0) {
            gbinder_client_unref (priv->m_clientGnss_2_0);
            priv->m_clientGnss_2_0 = NULL;
        }
    }
    if (priv->m_remoteGnss) {
        gbinder_remote_object_remove_handler (priv->m_remoteGnss, priv->m_death_id);
        gbinder_remote_object_unref (priv->m_remoteGnss);
        priv->m_death_id = 0;
        priv->m_remoteGnss = NULL;
    }
    if (priv->m_clientGnssDebug) {
        gbinder_client_unref (priv->m_clientGnssDebug);
        priv->m_clientGnssDebug = NULL;
    }
    if (priv->m_remoteGnssDebug) {
        gbinder_remote_object_unref (priv->m_remoteGnssDebug);
        priv->m_remoteGnssDebug = NULL;
    }
    if (priv->m_callbackGnssNi) {
        gbinder_local_object_drop (priv->m_callbackGnssNi);
        priv->m_callbackGnssNi = NULL;
    }
    if (priv->m_clientGnssNi) {
        gbinder_client_unref (priv->m_clientGnssNi);
        priv->m_clientGnssNi = NULL;
    }
    if (priv->m_remoteGnssNi) {
        gbinder_remote_object_unref (priv->m_remoteGnssNi);
        priv->m_remoteGnssNi = NULL;
    }
    if (priv->m_callbackGnssXtra) {
        gbinder_local_object_drop (priv->m_callbackGnssXtra);
        priv->m_callbackGnssXtra = NULL;
    }
    if (priv->m_clientGnssXtra) {
        gbinder_client_unref (priv->m_clientGnssXtra);
        priv->m_clientGnssXtra = NULL;
    }
    if (priv->m_remoteGnssXtra) {
        gbinder_remote_object_unref (priv->m_remoteGnssXtra);
        priv->m_remoteGnssXtra = NULL;
    }
    if (priv->m_callbackAGnss) {
        gbinder_local_object_drop (priv->m_callbackAGnss);
        priv->m_callbackAGnss = NULL;
    }
    if (priv->m_clientAGnss) {
        gbinder_client_unref (priv->m_clientAGnss);
        priv->m_clientAGnss = NULL;
    }
    if (priv->m_remoteAGnss) {
        gbinder_remote_object_unref (priv->m_remoteAGnss);
        priv->m_remoteAGnss = NULL;
    }
    if (priv->m_callbackAGnssRil) {
        gbinder_local_object_drop (priv->m_callbackAGnssRil);
        priv->m_callbackAGnssRil = NULL;
    }
    if (priv->m_clientAGnssRil) {
        gbinder_client_unref (priv->m_clientAGnssRil);
        priv->m_clientAGnssRil = NULL;
    }
    if (priv->m_remoteAGnssRil) {
        gbinder_remote_object_unref (priv->m_remoteAGnssRil);
        priv->m_remoteAGnssRil = NULL;
    }
    if (priv->m_sm) {
        gbinder_servicemanager_unref (priv->m_sm);
        priv->m_sm = NULL;
    }

    g_free (priv->m_fqname);
    priv->m_fqname = NULL;

    g_free (priv->m_fqname_2_0);
    priv->m_fqname_2_0 = NULL;
}

static
gboolean is_reply_success (GBinderRemoteReply *reply)
{
    GBinderReader reader;
    gint32 status;
    gboolean result;
    gbinder_remote_reply_init_reader (reply, &reader);

    if (!gbinder_reader_read_int32 (&reader, &status) || status != 0)
        return FALSE;

    if (!gbinder_reader_read_bool (&reader, &result) || !result)
        return FALSE;

    return TRUE;
}

static
GBinderRemoteObject* get_extension_object (GBinderRemoteReply *reply)
{
    GBinderReader reader;
    gint32 status;
    gbinder_remote_reply_init_reader (reply, &reader);

    if (!gbinder_reader_read_int32 (&reader, &status) || status != 0) {
        g_warning ("Failed to get extension object %d", status);
        return NULL;
    }

    return gbinder_reader_read_object (&reader);
}

static void
gclue_hybris_binder_init (GClueHybrisBinder *hbinder)
{
    hbinder->priv = G_TYPE_INSTANCE_GET_PRIVATE ((hbinder),
                                                 GCLUE_TYPE_HYBRIS_BINDER,
                                                 GClueHybrisBinderPrivate);
}

static void
on_hybris_binder_destroyed(gpointer data,
                           GObject *where_the_object_was)
{
    GClueHybrisBinder **hbinder = (GClueHybrisBinder **) data;

    gclue_hybris_binder_dropGnss (*hbinder);

    *hbinder = NULL;
}

GClueHybris*
gclue_hybris_binder_get_singleton (void)
{
    static GClueHybrisBinder *hbinder = NULL;

    if (hbinder == NULL) {
        hbinder = g_object_new (GCLUE_TYPE_HYBRIS_BINDER, NULL);
        g_object_weak_ref (G_OBJECT (hbinder),
                           on_hybris_binder_destroyed,
                           &hbinder);
    } else
        g_object_ref (hbinder);

    return GCLUE_HYBRIS (hbinder);
}

// Gnss
gboolean
gclue_hybris_binder_gnssInit (GClueHybris *hybris)
{
    GClueHybrisBinder *hbinder;
    GClueHybrisBinderPrivate *priv;
    g_return_val_if_fail (GCLUE_IS_HYBRIS_BINDER (hybris), FALSE);
    hbinder = GCLUE_HYBRIS_BINDER (hybris);
    priv = hbinder->priv;

    gboolean ret = FALSE;
    gboolean ntp_ret = FALSE;
    int64_t timeMs, timeReferenceMs;
    int32_t uncertaintyMs;
    const char *ntp;

    g_warning ("Initialising GNSS interface");

    priv->m_sm = gbinder_servicemanager_new (GNSS_BINDER_DEFAULT_DEV);
    if (priv->m_sm) {
        int status = 0;

        /* Fetch remote reference from hwservicemanager */
        priv->m_fqname = g_strconcat (GNSS_REMOTE "/default", NULL);
        priv->m_fqname_2_0 = g_strconcat (GNSS_REMOTE_2_0 "/default", NULL);

        priv->m_remoteGnss = gbinder_servicemanager_get_service_sync (priv->m_sm,
                                                                      priv->m_fqname, &status);

        if (service_exists (priv->m_sm, priv->m_fqname_2_0)) {
            g_debug ("Service %s exists", priv->m_fqname_2_0);
            priv->m_gnss2Available = 1;

            priv->m_remoteGnss_2_0 = gbinder_servicemanager_get_service_sync (priv->m_sm,
                                                                              priv->m_fqname_2_0, NULL);
        } else {
            g_debug ("Service %s does not exist", priv->m_fqname_2_0);
            priv->m_gnss2Available = 0;
        }

        if (priv->m_remoteGnss) {
            GBinderLocalRequest *req;
            GBinderRemoteReply *reply;

            /* get_service returns auto-released reference,
             * we need to add a reference of our own */
            gbinder_remote_object_ref (priv->m_remoteGnss);
            priv->m_clientGnss = gbinder_client_new (priv->m_remoteGnss, GNSS_REMOTE);
            priv->m_death_id = gbinder_remote_object_add_death_handler (priv->m_remoteGnss, geoclue_binder_gnss_gnss_died, hybris);
            priv->m_callbackGnss = gbinder_servicemanager_new_local_object (priv->m_sm, GNSS_CALLBACK, geoclue_binder_gnss_callback, hybris);

            /* IGnss::setCallback */
            req = gbinder_client_new_request (priv->m_clientGnss);
            gbinder_local_request_append_local_object (req, priv->m_callbackGnss);
            reply = gbinder_client_transact_sync_reply (priv->m_clientGnss,
                                                        GNSS_SET_CALLBACK, req, &status);

            if (priv->m_gnss2Available == 1) {
                if (priv->m_remoteGnss_2_0)
                    priv->m_clientGnss_2_0 = gbinder_client_new (priv->m_remoteGnss_2_0, GNSS_REMOTE_2_0);
                else
                    priv->m_gnss2Available = 1;
            }

            ntp = get_ntp_server_config ();
            int success = query_ntp_server (&timeMs, &uncertaintyMs, &timeReferenceMs, ntp);
            if (!success)
                g_debug ("Failed to query NTP server %s", ntp);
            else {
                g_debug ("Injecting epoch time %ld from NTP server %s", timeMs, ntp);
                ntp_ret = gclue_hybris_binder_gnssInjectTime (hybris, timeMs, timeReferenceMs, uncertaintyMs);
                if (!ntp_ret)
                    g_debug ("Failed to inject epoch time into GNSS modem");
            }

            if (!status)
                ret = is_reply_success (reply);

            gbinder_local_request_unref (req);
            gbinder_remote_reply_unref (reply);
        }
    }

    if (!ret)
        g_warning ("Failed to initialise GNSS interface");

    return ret;
}

gboolean
gclue_hybris_binder_gnssStart (GClueHybris *hybris)
{
    GClueHybrisBinder *hbinder;
    GClueHybrisBinderPrivate *priv;
    g_return_val_if_fail (GCLUE_IS_HYBRIS_BINDER (hybris), FALSE);
    hbinder = GCLUE_HYBRIS_BINDER (hybris);
    priv = hbinder->priv;

    gboolean ret = FALSE;

    if (priv->m_clientGnss) {
        int status = 0;
        GBinderRemoteReply *reply;

        reply = gbinder_client_transact_sync_reply (priv->m_clientGnss,
                                                    GNSS_START, NULL, &status);

        if (!status)
            ret = is_reply_success (reply);

        gbinder_remote_reply_unref (reply);
    }

    if (!ret)
        g_warning ("Failed to start positioning");

    return ret;
}

gboolean
gclue_hybris_binder_gnssStop (GClueHybris *hybris)
{
    GClueHybrisBinder *hbinder;
    GClueHybrisBinderPrivate *priv;
    g_return_val_if_fail (GCLUE_IS_HYBRIS_BINDER (hybris), FALSE);
    hbinder = GCLUE_HYBRIS_BINDER (hybris);
    priv = hbinder->priv;

    gboolean ret = FALSE;

    if (priv->m_clientGnss) {
        int status = 0;
        GBinderRemoteReply *reply;

        reply = gbinder_client_transact_sync_reply (priv->m_clientGnss,
                                                    GNSS_STOP, NULL, &status);

        if (!status)
            ret = is_reply_success (reply);

        gbinder_remote_reply_unref (reply);
    }

    if (!ret)
        g_warning ("Failed to stop positioning");

    return ret;
}

void
gclue_hybris_binder_gnssCleanup (GClueHybris *hybris)
{
    GClueHybrisBinder *hbinder;
    GClueHybrisBinderPrivate *priv;
    g_return_if_fail (GCLUE_IS_HYBRIS_BINDER (hybris));
    hbinder = GCLUE_HYBRIS_BINDER (hybris);
    priv = hbinder->priv;

    if (priv->m_clientGnss)
        gbinder_client_transact (priv->m_clientGnss, GNSS_CLEANUP, 0, NULL, NULL, NULL, NULL);
}

gboolean
gclue_hybris_binder_gnssInjectLocation (GClueHybris *hybris,
                                        double latitudeDegrees,
                                        double longitudeDegrees,
                                        float accuracyMeters)
{
    GClueHybrisBinder *hbinder;
    GClueHybrisBinderPrivate *priv;
    g_return_val_if_fail (GCLUE_IS_HYBRIS_BINDER (hybris), FALSE);
    hbinder = GCLUE_HYBRIS_BINDER (hybris);
    priv = hbinder->priv;

    gboolean ret = FALSE;

    if (priv->m_clientGnss) {
        int status = 0;

        GBinderLocalRequest *req;
        GBinderRemoteReply *reply;
        GBinderWriter writer;

        req = gbinder_client_new_request (priv->m_clientGnss);
        gbinder_local_request_init_writer (req, &writer);
        gbinder_writer_append_double (&writer, latitudeDegrees);
        gbinder_writer_append_double (&writer, longitudeDegrees);
        gbinder_writer_append_float (&writer, accuracyMeters);
        reply = gbinder_client_transact_sync_reply (priv->m_clientGnss,
                                                    GNSS_INJECT_LOCATION, req, &status);

        if (!status)
            ret = is_reply_success (reply);

        if (!ret)
            g_warning ("Failed to inject location");

        gbinder_local_request_unref (req);
        gbinder_remote_reply_unref (reply);
    }
    return ret;
}

gboolean
gclue_hybris_binder_gnssInjectTime (GClueHybris *hybris,
                                    HybrisGnssUtcTime timeMs,
                                    int64_t timeReferenceMs,
                                    int32_t uncertaintyMs)
{
    GClueHybrisBinder *hbinder;
    GClueHybrisBinderPrivate *priv;
    g_return_val_if_fail (GCLUE_IS_HYBRIS_BINDER (hybris), FALSE);
    hbinder = GCLUE_HYBRIS_BINDER (hybris);
    priv = hbinder->priv;

    gboolean ret = FALSE;

    if (priv->m_clientGnss) {
        int status = 0;
        GBinderLocalRequest *req;
        GBinderRemoteReply *reply;
        GBinderWriter writer;

        req = gbinder_client_new_request (priv->m_clientGnss);
        gbinder_local_request_init_writer (req, &writer);
        gbinder_writer_append_int64 (&writer, timeMs);
        gbinder_writer_append_int64 (&writer, timeReferenceMs);
        gbinder_writer_append_int32 (&writer, uncertaintyMs);

        reply = gbinder_client_transact_sync_reply (priv->m_clientGnss,
                                                    GNSS_INJECT_TIME, req, &status);

        if (!status)
            ret = is_reply_success (reply);

        if (!ret)
            g_warning ("Failed to inject time");

        gbinder_local_request_unref (req);
        gbinder_remote_reply_unref (reply);
    }
    return ret;
}

void
gclue_hybris_binder_gnssDeleteAidingData (GClueHybris *hybris,
                                          HybrisGnssAidingData aidingDataFlags)
{
    GClueHybrisBinder *hbinder;
    GClueHybrisBinderPrivate *priv;
    g_return_if_fail (GCLUE_IS_HYBRIS_BINDER (hybris));
    hbinder = GCLUE_HYBRIS_BINDER (hybris);
    priv = hbinder->priv;

    if (priv->m_clientGnss) {
        GBinderLocalRequest *req;

        req = gbinder_client_new_request (priv->m_clientGnss);
        gbinder_local_request_append_int32 (req, aidingDataFlags);
        gbinder_client_transact (priv->m_clientGnss, GNSS_DELETE_AIDING_DATA,
                                 0, req, NULL, NULL, NULL);

        gbinder_local_request_unref (req);
    }
}

gboolean
gclue_hybris_binder_gnssSetPositionMode (GClueHybris *hybris,
                                         HybrisGnssPositionMode mode,
                                         HybrisGnssPositionRecurrence recurrence,
                                         guint32 minIntervalMs,
                                         guint32 preferredAccuracyMeters,
                                         guint32 preferredTimeMs)
{
    GClueHybrisBinder *hbinder;
    GClueHybrisBinderPrivate *priv;
    g_return_val_if_fail (GCLUE_IS_HYBRIS_BINDER (hybris), FALSE);
    hbinder = GCLUE_HYBRIS_BINDER (hybris);
    priv = hbinder->priv;

    gboolean ret = FALSE;

    if (priv->m_clientGnss) {
        int status = 0;
        GBinderLocalRequest *req;
        GBinderRemoteReply *reply;
        GBinderWriter writer;

        req = gbinder_client_new_request (priv->m_clientGnss);
        gbinder_local_request_init_writer (req, &writer);
        gbinder_writer_append_int32 (&writer, mode);
        gbinder_writer_append_int32 (&writer, recurrence);
        gbinder_writer_append_int32 (&writer, minIntervalMs);
        gbinder_writer_append_int32 (&writer, preferredAccuracyMeters);
        gbinder_writer_append_int32 (&writer, preferredTimeMs);
        reply = gbinder_client_transact_sync_reply (priv->m_clientGnss,
                                                    GNSS_SET_POSITION_MODE, req, &status);

        if (!status)
            ret = is_reply_success (reply);

        if (!ret)
            g_warning ("GNSS set position mode failed");

        gbinder_local_request_unref (req);
        gbinder_remote_reply_unref (reply);
    }
    return ret;
}

// GnssDebug
void
gclue_hybris_binder_gnssDebugInit (GClueHybris *hybris)
{
    GClueHybrisBinder *hbinder;
    GClueHybrisBinderPrivate *priv;
    g_return_if_fail (GCLUE_IS_HYBRIS_BINDER (hybris));
    hbinder = GCLUE_HYBRIS_BINDER (hybris);
    priv = hbinder->priv;

    GBinderRemoteReply *reply;
    int status = 0;

    reply = gbinder_client_transact_sync_reply (priv->m_clientGnss,
                                                GNSS_GET_EXTENSION_GNSS_DEBUG, NULL, &status);

    if (!status) {
        priv->m_remoteGnssDebug = get_extension_object (reply);
        if (priv->m_remoteGnssDebug) {
            g_warning ("Initialising GNSS Debug interface");
            priv->m_clientGnssDebug = gbinder_client_new (priv->m_remoteGnssDebug, GNSS_DEBUG_REMOTE);
        }
    }
    gbinder_remote_reply_unref(reply);
}

// GnssNi
void
gclue_hybris_binder_gnssNiInit (GClueHybris *hybris)
{
    GClueHybrisBinder *hbinder;
    GClueHybrisBinderPrivate *priv;
    g_return_if_fail (GCLUE_IS_HYBRIS_BINDER (hybris));
    hbinder = GCLUE_HYBRIS_BINDER (hybris);
    priv = hbinder->priv;

    GBinderRemoteReply *reply;
    int status = 0;

    reply = gbinder_client_transact_sync_reply (priv->m_clientGnss,
                                                GNSS_GET_EXTENSION_GNSS_NI, NULL, &status);

    if (!status) {
        priv->m_remoteGnssNi = get_extension_object(reply);

        if (priv->m_remoteGnssNi) {
            g_warning ("Initialising GNSS NI interface");
            GBinderLocalRequest *req;
            priv->m_clientGnssNi = gbinder_client_new (priv->m_remoteGnssNi, GNSS_NI_REMOTE);
            priv->m_callbackGnssNi = gbinder_servicemanager_new_local_object (priv->m_sm, GNSS_NI_CALLBACK, geoclue_binder_gnss_ni_callback, hybris);

            gbinder_remote_reply_unref (reply);

            /* IGnssNi::setCallback */
            req = gbinder_client_new_request (priv->m_clientGnssNi);
            gbinder_local_request_append_local_object (req, priv->m_callbackGnssNi);
            reply = gbinder_client_transact_sync_reply (priv->m_clientGnssNi,
                                                        GNSS_NI_SET_CALLBACK, req, &status);

            if (!status) {
                if (!gbinder_remote_reply_read_int32 (reply, &status) || status != 0)
                    g_warning ("Initialising GNSS NI interface failed %d", status);
            }

            gbinder_local_request_unref(req);
        }
    }
    gbinder_remote_reply_unref(reply);
}

void
gclue_hybris_binder_gnssNiRespond (GClueHybris *hybris,
                                   int32_t notifId,
                                   HybrisGnssUserResponseType userResponse)
{
    GClueHybrisBinder *hbinder;
    GClueHybrisBinderPrivate *priv;
    g_return_if_fail (GCLUE_IS_HYBRIS_BINDER (hybris));
    hbinder = GCLUE_HYBRIS_BINDER (hybris);
    priv = hbinder->priv;

    if (priv->m_clientGnssNi) {
        int status = 0;
        GBinderLocalRequest *req;
        GBinderRemoteReply *reply;
        GBinderWriter writer;

        req = gbinder_client_new_request (priv->m_clientGnssNi);
        gbinder_local_request_init_writer (req, &writer);
        gbinder_writer_append_int32 (&writer, notifId);
        gbinder_writer_append_int32 (&writer, userResponse);

        reply = gbinder_client_transact_sync_reply (priv->m_clientGnssNi,
                                                    GNSS_NI_RESPOND, req, &status);

        if (!status) {
            if (!gbinder_remote_reply_read_int32 (reply, &status) || status != 0)
                g_warning ("GNSS NI respond failed %d", status);
        }

        gbinder_local_request_unref (req);
        gbinder_remote_reply_unref (reply);
    }
}

// GnssXtra
void
gclue_hybris_binder_gnssXtraInit (GClueHybris *hybris)
{
    GClueHybrisBinder *hbinder;
    GClueHybrisBinderPrivate *priv;
    g_return_if_fail (GCLUE_IS_HYBRIS_BINDER (hybris));
    hbinder = GCLUE_HYBRIS_BINDER (hybris);
    priv = hbinder->priv;

    GBinderRemoteReply *reply;
    int status = 0;

    reply = gbinder_client_transact_sync_reply (priv->m_clientGnss,
                                                GNSS_GET_EXTENSION_XTRA, NULL, &status);

    if (!status) {
        priv->m_remoteGnssXtra = get_extension_object (reply);

        if (priv->m_remoteGnssXtra) {
            g_warning ("Initialising GNSS Xtra interface");
            GBinderLocalRequest *req;
            priv->m_clientGnssXtra = gbinder_client_new (priv->m_remoteGnssXtra, GNSS_XTRA_REMOTE);
            priv->m_callbackGnssXtra = gbinder_servicemanager_new_local_object (priv->m_sm, GNSS_XTRA_CALLBACK, geoclue_binder_gnss_xtra_callback, hybris);

            gbinder_remote_reply_unref (reply);

            /* IGnssXtra::setCallback */
            req = gbinder_client_new_request (priv->m_clientGnssXtra);
            gbinder_local_request_append_local_object (req, priv->m_callbackGnssXtra);
            reply = gbinder_client_transact_sync_reply (priv->m_clientGnssXtra,
                                                        GNSS_XTRA_SET_CALLBACK, req, &status);

            if (status || !is_reply_success (reply))
                g_warning ("Initialising GNSS Xtra interface failed");

            gbinder_local_request_unref (req);
        }
    }
    gbinder_remote_reply_unref (reply);
}

gboolean
gclue_hybris_binder_gnssXtraInjectXtraData (GClueHybris *hybris,
                                            gchar *xtraData)
{
    GClueHybrisBinder *hbinder;
    GClueHybrisBinderPrivate *priv;
    g_return_val_if_fail (GCLUE_IS_HYBRIS_BINDER (hybris), FALSE);
    hbinder = GCLUE_HYBRIS_BINDER (hybris);
    priv = hbinder->priv;

    gboolean ret = FALSE;
    if (priv->m_clientGnssXtra) {
        int status = 0;

        GBinderLocalRequest *req;
        GBinderRemoteReply *reply;

        req = gbinder_client_new_request (priv->m_clientGnssXtra);
        gbinder_local_request_append_hidl_string (req, xtraData);
        reply = gbinder_client_transact_sync_reply (priv->m_clientGnssXtra,
                                                    GNSS_XTRA_INJECT_XTRA_DATA, req, &status);

        if (!status)
            ret = is_reply_success (reply);

        if (!ret)
            g_warning ("GNSS Xtra inject xtra data failed");

        gbinder_local_request_unref (req);
        gbinder_remote_reply_unref (reply);
    }
    return ret;
}

// AGnss
void
gclue_hybris_binder_aGnssInit (GClueHybris *hybris)
{
    GClueHybrisBinder *hbinder;
    GClueHybrisBinderPrivate *priv;
    g_return_if_fail (GCLUE_IS_HYBRIS_BINDER (hybris));
    hbinder = GCLUE_HYBRIS_BINDER (hybris);
    priv = hbinder->priv;

    GBinderRemoteReply *reply;
    int status = 0;
    gboolean supl_ret = FALSE;
    const char *supl = NULL;

    if (priv->m_gnss2Available)
        reply = gbinder_client_transact_sync_reply (priv->m_clientGnss_2_0, GNSS_GET_EXTENSION_AGNSS_2_0, NULL, &status);
    else
        reply = gbinder_client_transact_sync_reply (priv->m_clientGnss, GNSS_GET_EXTENSION_AGNSS, NULL, &status);

    if (!status) {
        priv->m_remoteAGnss = get_extension_object (reply);

        if (priv->m_remoteAGnss) {
            g_warning ("Initialising AGNSS interface");
            GBinderLocalRequest *req;

            if (priv->m_gnss2Available)
                priv->m_clientAGnss = gbinder_client_new (priv->m_remoteAGnss, AGNSS_REMOTE_2_0);
            else
                priv->m_clientAGnss = gbinder_client_new (priv->m_remoteAGnss, AGNSS_REMOTE);

            priv->m_callbackAGnss = gbinder_servicemanager_new_local_object (priv->m_sm, AGNSS_CALLBACK, geoclue_binder_agnss_callback, hybris);

            gbinder_remote_reply_unref (reply);

            /* IAGnss::setCallback */
            req = gbinder_client_new_request (priv->m_clientAGnss);
            gbinder_local_request_append_local_object (req, priv->m_callbackAGnss);
            reply = gbinder_client_transact_sync_reply (priv->m_clientAGnss,
                                                        AGNSS_SET_CALLBACK, req, &status);

            if (!status) {
                if (!gbinder_remote_reply_read_int32 (reply, &status) || status != 0)
                    g_warning ("Initialising AGNSS interface failed %d", status);
            }


            supl = get_supl_server_config ();
            if (supl != NULL) {
                char *supl_domain = NULL;
                int supl_port = 0;

                supl_ret = parse_supl_string (supl, &supl_domain, &supl_port);
                if (supl_ret) {
                    supl_ret = gclue_hybris_binder_aGnssSetServer (hybris, HYBRIS_APN_IP_IPV4, supl_domain, supl_port);
                    if (supl_ret)
                        g_debug ("SUPL server %s:%d has been set successfully", supl_domain, supl_port);
                    else
                        g_debug ("Failed to set %s:%d SUPL server", supl_domain, supl_port);

                    g_free (supl_domain);
                }
            }

            gbinder_local_request_unref (req);
        }
    }
    gbinder_remote_reply_unref (reply);
}

gboolean
gclue_hybris_binder_aGnssDataConnClosed (GClueHybris *hybris)
{
    GClueHybrisBinder *hbinder;
    GClueHybrisBinderPrivate *priv;
    g_return_val_if_fail (GCLUE_IS_HYBRIS_BINDER (hybris), FALSE);
    hbinder = GCLUE_HYBRIS_BINDER (hybris);
    priv = hbinder->priv;

    int status = 0;
    gboolean ret = FALSE;
    GBinderRemoteReply *reply;

    reply = gbinder_client_transact_sync_reply (priv->m_clientAGnss,
                                                AGNSS_DATA_CONN_CLOSED, NULL, &status);

    if (!status)
        ret = is_reply_success (reply);

    gbinder_remote_reply_unref (reply);
    return ret;
}

gboolean
gclue_hybris_binder_aGnssDataConnFailed (GClueHybris *hybris)
{
    GClueHybrisBinder *hbinder;
    GClueHybrisBinderPrivate *priv;
    g_return_val_if_fail (GCLUE_IS_HYBRIS_BINDER (hybris), FALSE);
    hbinder = GCLUE_HYBRIS_BINDER (hybris);
    priv = hbinder->priv;

    int status = 0;
    gboolean ret = FALSE;
    GBinderRemoteReply *reply;

    reply = gbinder_client_transact_sync_reply (priv->m_clientAGnss,
                                                AGNSS_DATA_CONN_FAILED, NULL, &status);

    if (!status)
        ret = is_reply_success (reply);

    gbinder_remote_reply_unref (reply);
    return ret;
}

gboolean
gclue_hybris_binder_aGnssDataConnOpen (GClueHybris *hybris,
                                       const char* apn,
                                       const char* protocol)
{
    GClueHybrisBinder *hbinder;
    GClueHybrisBinderPrivate *priv;
    g_return_val_if_fail (GCLUE_IS_HYBRIS_BINDER (hybris), FALSE);
    hbinder = GCLUE_HYBRIS_BINDER (hybris);
    priv = hbinder->priv;

    int status = 0;
    gboolean ret = FALSE;
    GBinderLocalRequest *req;
    GBinderRemoteReply *reply;
    GBinderWriter writer;

    req = gbinder_client_new_request (priv->m_clientAGnss);

    gbinder_local_request_init_writer (req, &writer);
    gbinder_writer_append_hidl_string (&writer, apn);
    gbinder_writer_append_int32 (&writer, protocol_to_apn_type (protocol));
    reply = gbinder_client_transact_sync_reply (priv->m_clientAGnss,
                                                AGNSS_DATA_CONN_OPEN, req, &status);

    if (!status)
        ret = is_reply_success (reply);

    gbinder_local_request_unref (req);
    gbinder_remote_reply_unref (reply);

    return ret;
}

gboolean
gclue_hybris_binder_aGnssSetServer (GClueHybris *hybris,
                                    HybrisAGnssType type,
                                    const char *hostname,
                                    int port)
{
    GClueHybrisBinder *hbinder;
    GClueHybrisBinderPrivate *priv;
    g_return_val_if_fail (GCLUE_IS_HYBRIS_BINDER (hybris), FALSE);
    hbinder = GCLUE_HYBRIS_BINDER (hybris);
    priv = hbinder->priv;

    int status = 0;
    gboolean ret = FALSE;
    GBinderLocalRequest *req;
    GBinderRemoteReply *reply;
    GBinderWriter writer;

    req = gbinder_client_new_request (priv->m_clientAGnss);

    gbinder_local_request_init_writer (req, &writer);
    gbinder_writer_append_int32 (&writer, type);
    gbinder_writer_append_hidl_string (&writer, hostname);
    gbinder_writer_append_int32 (&writer, port);
    reply = gbinder_client_transact_sync_reply (priv->m_clientAGnss,
                                                AGNSS_SET_SERVER, req, &status);

    if (!status)
        ret = is_reply_success (reply);

    gbinder_local_request_unref (req);
    gbinder_remote_reply_unref (reply);

    return ret;
}

// AGnssRil
void
gclue_hybris_binder_aGnssRilInit (GClueHybris *hybris)
{
    GClueHybrisBinder *hbinder;
    GClueHybrisBinderPrivate *priv;
    g_return_if_fail (GCLUE_IS_HYBRIS_BINDER (hybris));
    hbinder = GCLUE_HYBRIS_BINDER (hybris);
    priv = hbinder->priv;

    GBinderRemoteReply *reply;
    int status = 0;
    gboolean setid_ret;

    if (priv->m_gnss2Available)
        reply = gbinder_client_transact_sync_reply (priv->m_clientGnss_2_0, GNSS_GET_EXTENSION_AGNSS_RIL_2_0, NULL, &status);
    else
        reply = gbinder_client_transact_sync_reply (priv->m_clientGnss, GNSS_GET_EXTENSION_AGNSS_RIL, NULL, &status);

    if (!status) {
        priv->m_remoteAGnssRil = get_extension_object (reply);

        if (priv->m_remoteAGnssRil) {
            g_warning ("Initialising AGNSS RIL interface");
            GBinderLocalRequest *req;

            if (priv->m_gnss2Available)
                priv->m_clientAGnssRil = gbinder_client_new (priv->m_remoteAGnssRil, AGNSS_RIL_REMOTE_2_0);
            else
                priv->m_clientAGnssRil = gbinder_client_new (priv->m_remoteAGnssRil, AGNSS_RIL_REMOTE);

            priv->m_clientAGnssRil = gbinder_client_new (priv->m_remoteAGnssRil, AGNSS_RIL_REMOTE);
            priv->m_callbackAGnssRil = gbinder_servicemanager_new_local_object (priv->m_sm, AGNSS_RIL_CALLBACK, geoclue_binder_agnss_ril_callback, hybris);

            gbinder_remote_reply_unref (reply);

            /* IAGnssRil::setCallback */
            req = gbinder_client_new_request (priv->m_clientAGnssRil);
            gbinder_local_request_append_local_object (req, priv->m_callbackAGnssRil);
            reply = gbinder_client_transact_sync_reply (priv->m_clientAGnssRil,
                                                        AGNSS_RIL_SET_CALLBACK, req, &status);

            if (!status) {
                if (!gbinder_remote_reply_read_int32 (reply, &status) || status != 0)
                    g_warning ("Initialising AGNSS RIL interface failed %d", status);
            }

            char *subscriber_identity = NULL;
            if (get_subscriber_identity (&subscriber_identity) == 1) {
                g_debug ("Setting AGNSS RIL IMSI %s", subscriber_identity);
                setid_ret = gclue_hybris_binder_aGnssRilsetSetId (hybris, SETID_IMSI, subscriber_identity);

                if (setid_ret)
                     g_debug ("Successfully set AGNSS RIL IMSI to %s", subscriber_identity);

                free (subscriber_identity);
            }

            gbinder_local_request_unref (req);
        }
    }
    gbinder_remote_reply_unref (reply);
}

gboolean
gclue_hybris_binder_aGnssRilsetSetId (GClueHybris *hybris,
                                      int type,
                                      const char *setid)
{
    GClueHybrisBinder *hbinder;
    GClueHybrisBinderPrivate *priv;
    g_return_val_if_fail (GCLUE_IS_HYBRIS_BINDER (hybris), FALSE);
    hbinder = GCLUE_HYBRIS_BINDER (hybris);
    priv = hbinder->priv;

    int status = 0;
    gboolean ret = FALSE;
    GBinderLocalRequest *req;
    GBinderRemoteReply *reply;
    GBinderWriter writer;

    req = gbinder_client_new_request (priv->m_clientAGnssRil);

    gbinder_local_request_init_writer (req, &writer);
    gbinder_writer_append_int32 (&writer, type);
    gbinder_writer_append_hidl_string (&writer, setid);
    reply = gbinder_client_transact_sync_reply (priv->m_clientAGnssRil,
                                                AGNSS_RIL_SET_SET_ID, req, &status);

    if (!status)
        ret = is_reply_success (reply);

    gbinder_local_request_unref (req);
    gbinder_remote_reply_unref (reply);

    return ret;
}
