/*
 * Geoclue
 * master.c - Master process
 *
 * Authors: Iain Holmes <iain@openedhand.com>
 *          Jussi Kukkonen <jku@o-hand.com>
 * Copyright 2007-2008 by Garmin Ltd. or its subsidiaries
 */

#include <config.h>

#include <string.h>

#include "main.h"
#include "master.h"
#include "client.h"
#include "master-provider.h"

#ifdef HAVE_NETWORK_MANAGER
#include "connectivity-networkmanager.h"
#else
#ifdef HAVE_CONIC
#include "connectivity-conic.h"
#endif
#endif

G_DEFINE_TYPE (GcMaster, gc_master, G_TYPE_OBJECT);

static GList *providers = NULL;

static gboolean gc_iface_master_create (GcMaster    *master,
					const char **object_path,
					GError     **error);

#include "gc-iface-master-glue.h"

#define GEOCLUE_MASTER_PATH "/org/freedesktop/Geoclue/Master/client"
static gboolean
gc_iface_master_create (GcMaster    *master,
			const char **object_path,
			GError     **error)
{
	static guint32 serial = 0;
	GcMasterClient *client;
	char *path;

	path = g_strdup_printf ("%s%d", GEOCLUE_MASTER_PATH, serial++);
	client = g_object_new (GC_TYPE_MASTER_CLIENT, NULL);
	dbus_g_connection_register_g_object (master->connection, path,
					     G_OBJECT (client));
	
	if (object_path) {
		*object_path = path;
	}
	return TRUE;
}

static void
gc_master_class_init (GcMasterClass *klass)
{
	dbus_g_object_type_install_info (gc_master_get_type (),
					 &dbus_glib_gc_iface_master_object_info);
}

/* Load the provider details out of a keyfile */
static void
gc_master_add_new_provider (GcMaster   *master,
                            const char *filename)
{
	GcMasterProvider *provider;
	
	provider = gc_master_provider_new (filename, 
	                                   master->connectivity);
	
	if (!provider) {
		g_warning ("Loading from %s failed", filename);
		return;
	}
	
	providers = g_list_prepend (providers, provider);
}

/* Scan a directory for .provider files */
#define PROVIDER_EXTENSION ".provider"

static void
gc_master_load_providers (GcMaster *master)
{
	GDir *dir;
	GError *error = NULL;
	const char *filename;

	dir = g_dir_open (GEOCLUE_PROVIDERS_DIR, 0, &error);
	if (dir == NULL) {
		g_warning ("Error opening %s: %s\n", GEOCLUE_PROVIDERS_DIR,
			   error->message);
		g_error_free (error);
		return;
	}

	filename = g_dir_read_name (dir);
	if (!filename) {
		g_print ("No providers found in %s\n", dir);
	} else {
		g_print ("Found providers:\n");
	}
	while (filename) {
		char *fullname, *ext;

		g_print ("  %s\n", filename);
		ext = strrchr (filename, '.');
		if (ext == NULL || strcmp (ext, PROVIDER_EXTENSION) != 0) {
			g_print ("   - Ignored\n");
			filename = g_dir_read_name (dir);
			continue;
		}

		fullname = g_build_filename (GEOCLUE_PROVIDERS_DIR, 
					     filename, NULL);
		gc_master_add_new_provider (master, fullname);
		g_free (fullname);
		
		filename = g_dir_read_name (dir);
	}

	g_dir_close (dir);
}

static void
gc_master_init (GcMaster *master)
{
	GError *error = NULL;
	
	
	master->connection = dbus_g_bus_get (GEOCLUE_DBUS_BUS, &error);
	if (master->connection == NULL) {
		g_warning ("Could not get %s: %s", GEOCLUE_DBUS_BUS, 
			   error->message);
		g_error_free (error);
	}
	
	master->connectivity = NULL;
#ifdef HAVE_NETWORK_MANAGER
	master->connectivity = GEOCLUE_CONNECTIVITY (g_object_new (GEOCLUE_TYPE_NETWORKMANAGER, NULL));
#else
#ifdef HAVE_CONIC
	master->connectivity = GEOCLUE_CONNECTIVITY (g_object_new (GEOCLUE_TYPE_CONIC, NULL));
#endif
#endif
	
	gc_master_load_providers (master);
}


GList *
gc_master_get_providers (GcInterfaceFlags      iface_type,
                         GeoclueAccuracyLevel  min_accuracy,
                         gboolean              can_update,
                         GeoclueResourceFlags  allowed,
                         GError              **error)
{
	GList *l, *p = NULL;
	
	if (providers == NULL) {
		return NULL;
	}
	
	for (l = providers; l; l = l->next) {
		GcMasterProvider *provider = l->data;
		
		if (gc_master_provider_is_good (provider,
		                                iface_type, 
		                                min_accuracy, 
		                                can_update, 
		                                allowed)) {
			p = g_list_prepend (p, provider);
		}
	}
	
	return p;
}
