/*
 * Geoclue
 * geoclue-connectivity.h 
 *
 * Author: Jussi Kukkonen <jku@o-hand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 */

#ifndef _GEOCLUE_CONNECTIVITY_H
#define _GEOCLUE_CONNECTIVITY_H

#include <glib-object.h>
#include <geoclue/geoclue-types.h>

G_BEGIN_DECLS


#define GEOCLUE_TYPE_CONNECTIVITY (geoclue_connectivity_get_type ())
#define GEOCLUE_CONNECTIVITY(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_CONNECTIVITY, GeoclueConnectivity))
#define GEOCLUE_IS_CONNECTIVITY(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEOCLUE_TYPE_CONNECTIVITY))
#define GEOCLUE_CONNECTIVITY_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), GEOCLUE_TYPE_CONNECTIVITY, GeoclueConnectivityInterface))

typedef struct _GeoclueConnectivity GeoclueConnectivity;
typedef struct _GeoclueConnectivityInterface GeoclueConnectivityInterface;

struct _GeoclueConnectivityInterface {
	GTypeInterface parent;
	
	/* signals */
	void (* status_changed) (GeoclueConnectivity *self,
	                         GeoclueNetworkStatus status);
	
	/* vtable */
	int (*get_status) (GeoclueConnectivity *self);
};

GType geoclue_connectivity_get_type (void);


GeoclueNetworkStatus geoclue_connectivity_get_status (GeoclueConnectivity *self);

void
geoclue_connectivity_emit_status_changed (GeoclueConnectivity *self,
                                          GeoclueNetworkStatus status);

G_END_DECLS

#endif
