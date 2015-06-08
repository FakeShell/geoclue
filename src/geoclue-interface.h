/*
 * Generated by gdbus-codegen 2.42.2. DO NOT EDIT.
 *
 * The license of this code is the same as for the source it was derived from.
 */

#ifndef __GEOCLUE_INTERFACE_H__
#define __GEOCLUE_INTERFACE_H__

#include <gio/gio.h>

G_BEGIN_DECLS


/* ------------------------------------------------------------------------ */
/* Declarations for org.freedesktop.GeoClue2.Manager */

#define GCLUE_DBUS_TYPE_MANAGER (gclue_dbus_manager_get_type ())
#define GCLUE_DBUS_MANAGER(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), GCLUE_DBUS_TYPE_MANAGER, GClueDBusManager))
#define GCLUE_DBUS_IS_MANAGER(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GCLUE_DBUS_TYPE_MANAGER))
#define GCLUE_DBUS_MANAGER_GET_IFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE ((o), GCLUE_DBUS_TYPE_MANAGER, GClueDBusManagerIface))

struct _GClueDBusManager;
typedef struct _GClueDBusManager GClueDBusManager;
typedef struct _GClueDBusManagerIface GClueDBusManagerIface;

struct _GClueDBusManagerIface
{
  GTypeInterface parent_iface;


  gboolean (*handle_add_agent) (
    GClueDBusManager *object,
    GDBusMethodInvocation *invocation,
    const gchar *arg_id);

  gboolean (*handle_get_client) (
    GClueDBusManager *object,
    GDBusMethodInvocation *invocation);

  guint  (*get_available_accuracy_level) (GClueDBusManager *object);

  gboolean  (*get_in_use) (GClueDBusManager *object);

};

GType gclue_dbus_manager_get_type (void) G_GNUC_CONST;

GDBusInterfaceInfo *gclue_dbus_manager_interface_info (void);
guint gclue_dbus_manager_override_properties (GObjectClass *klass, guint property_id_begin);


/* D-Bus method call completion functions: */
void gclue_dbus_manager_complete_get_client (
    GClueDBusManager *object,
    GDBusMethodInvocation *invocation,
    const gchar *client);

void gclue_dbus_manager_complete_add_agent (
    GClueDBusManager *object,
    GDBusMethodInvocation *invocation);



/* D-Bus method calls: */
void gclue_dbus_manager_call_get_client (
    GClueDBusManager *proxy,
    GCancellable *cancellable,
    GAsyncReadyCallback callback,
    gpointer user_data);

gboolean gclue_dbus_manager_call_get_client_finish (
    GClueDBusManager *proxy,
    gchar **out_client,
    GAsyncResult *res,
    GError **error);

gboolean gclue_dbus_manager_call_get_client_sync (
    GClueDBusManager *proxy,
    gchar **out_client,
    GCancellable *cancellable,
    GError **error);

void gclue_dbus_manager_call_add_agent (
    GClueDBusManager *proxy,
    const gchar *arg_id,
    GCancellable *cancellable,
    GAsyncReadyCallback callback,
    gpointer user_data);

gboolean gclue_dbus_manager_call_add_agent_finish (
    GClueDBusManager *proxy,
    GAsyncResult *res,
    GError **error);

gboolean gclue_dbus_manager_call_add_agent_sync (
    GClueDBusManager *proxy,
    const gchar *arg_id,
    GCancellable *cancellable,
    GError **error);



/* D-Bus property accessors: */
gboolean gclue_dbus_manager_get_in_use (GClueDBusManager *object);
void gclue_dbus_manager_set_in_use (GClueDBusManager *object, gboolean value);

guint gclue_dbus_manager_get_available_accuracy_level (GClueDBusManager *object);
void gclue_dbus_manager_set_available_accuracy_level (GClueDBusManager *object, guint value);


/* ---- */

#define GCLUE_DBUS_TYPE_MANAGER_PROXY (gclue_dbus_manager_proxy_get_type ())
#define GCLUE_DBUS_MANAGER_PROXY(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), GCLUE_DBUS_TYPE_MANAGER_PROXY, GClueDBusManagerProxy))
#define GCLUE_DBUS_MANAGER_PROXY_CLASS(k) (G_TYPE_CHECK_CLASS_CAST ((k), GCLUE_DBUS_TYPE_MANAGER_PROXY, GClueDBusManagerProxyClass))
#define GCLUE_DBUS_MANAGER_PROXY_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GCLUE_DBUS_TYPE_MANAGER_PROXY, GClueDBusManagerProxyClass))
#define GCLUE_DBUS_IS_MANAGER_PROXY(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GCLUE_DBUS_TYPE_MANAGER_PROXY))
#define GCLUE_DBUS_IS_MANAGER_PROXY_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), GCLUE_DBUS_TYPE_MANAGER_PROXY))

typedef struct _GClueDBusManagerProxy GClueDBusManagerProxy;
typedef struct _GClueDBusManagerProxyClass GClueDBusManagerProxyClass;
typedef struct _GClueDBusManagerProxyPrivate GClueDBusManagerProxyPrivate;

struct _GClueDBusManagerProxy
{
  /*< private >*/
  GDBusProxy parent_instance;
  GClueDBusManagerProxyPrivate *priv;
};

struct _GClueDBusManagerProxyClass
{
  GDBusProxyClass parent_class;
};

GType gclue_dbus_manager_proxy_get_type (void) G_GNUC_CONST;

void gclue_dbus_manager_proxy_new (
    GDBusConnection     *connection,
    GDBusProxyFlags      flags,
    const gchar         *name,
    const gchar         *object_path,
    GCancellable        *cancellable,
    GAsyncReadyCallback  callback,
    gpointer             user_data);
GClueDBusManager *gclue_dbus_manager_proxy_new_finish (
    GAsyncResult        *res,
    GError             **error);
GClueDBusManager *gclue_dbus_manager_proxy_new_sync (
    GDBusConnection     *connection,
    GDBusProxyFlags      flags,
    const gchar         *name,
    const gchar         *object_path,
    GCancellable        *cancellable,
    GError             **error);

void gclue_dbus_manager_proxy_new_for_bus (
    GBusType             bus_type,
    GDBusProxyFlags      flags,
    const gchar         *name,
    const gchar         *object_path,
    GCancellable        *cancellable,
    GAsyncReadyCallback  callback,
    gpointer             user_data);
GClueDBusManager *gclue_dbus_manager_proxy_new_for_bus_finish (
    GAsyncResult        *res,
    GError             **error);
GClueDBusManager *gclue_dbus_manager_proxy_new_for_bus_sync (
    GBusType             bus_type,
    GDBusProxyFlags      flags,
    const gchar         *name,
    const gchar         *object_path,
    GCancellable        *cancellable,
    GError             **error);


/* ---- */

#define GCLUE_DBUS_TYPE_MANAGER_SKELETON (gclue_dbus_manager_skeleton_get_type ())
#define GCLUE_DBUS_MANAGER_SKELETON(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), GCLUE_DBUS_TYPE_MANAGER_SKELETON, GClueDBusManagerSkeleton))
#define GCLUE_DBUS_MANAGER_SKELETON_CLASS(k) (G_TYPE_CHECK_CLASS_CAST ((k), GCLUE_DBUS_TYPE_MANAGER_SKELETON, GClueDBusManagerSkeletonClass))
#define GCLUE_DBUS_MANAGER_SKELETON_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GCLUE_DBUS_TYPE_MANAGER_SKELETON, GClueDBusManagerSkeletonClass))
#define GCLUE_DBUS_IS_MANAGER_SKELETON(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GCLUE_DBUS_TYPE_MANAGER_SKELETON))
#define GCLUE_DBUS_IS_MANAGER_SKELETON_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), GCLUE_DBUS_TYPE_MANAGER_SKELETON))

typedef struct _GClueDBusManagerSkeleton GClueDBusManagerSkeleton;
typedef struct _GClueDBusManagerSkeletonClass GClueDBusManagerSkeletonClass;
typedef struct _GClueDBusManagerSkeletonPrivate GClueDBusManagerSkeletonPrivate;

struct _GClueDBusManagerSkeleton
{
  /*< private >*/
  GDBusInterfaceSkeleton parent_instance;
  GClueDBusManagerSkeletonPrivate *priv;
};

struct _GClueDBusManagerSkeletonClass
{
  GDBusInterfaceSkeletonClass parent_class;
};

GType gclue_dbus_manager_skeleton_get_type (void) G_GNUC_CONST;

GClueDBusManager *gclue_dbus_manager_skeleton_new (void);


/* ------------------------------------------------------------------------ */
/* Declarations for org.freedesktop.GeoClue2.Client */

#define GCLUE_DBUS_TYPE_CLIENT (gclue_dbus_client_get_type ())
#define GCLUE_DBUS_CLIENT(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), GCLUE_DBUS_TYPE_CLIENT, GClueDBusClient))
#define GCLUE_DBUS_IS_CLIENT(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GCLUE_DBUS_TYPE_CLIENT))
#define GCLUE_DBUS_CLIENT_GET_IFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE ((o), GCLUE_DBUS_TYPE_CLIENT, GClueDBusClientIface))

struct _GClueDBusClient;
typedef struct _GClueDBusClient GClueDBusClient;
typedef struct _GClueDBusClientIface GClueDBusClientIface;

struct _GClueDBusClientIface
{
  GTypeInterface parent_iface;



  gboolean (*handle_start) (
    GClueDBusClient *object,
    GDBusMethodInvocation *invocation);

  gboolean (*handle_stop) (
    GClueDBusClient *object,
    GDBusMethodInvocation *invocation);

  gboolean  (*get_active) (GClueDBusClient *object);

  const gchar * (*get_desktop_id) (GClueDBusClient *object);

  guint  (*get_distance_threshold) (GClueDBusClient *object);

  const gchar * (*get_location) (GClueDBusClient *object);

  guint  (*get_requested_accuracy_level) (GClueDBusClient *object);

  void (*location_updated) (
    GClueDBusClient *object,
    const gchar *arg_old,
    const gchar *arg_new);

};

GType gclue_dbus_client_get_type (void) G_GNUC_CONST;

GDBusInterfaceInfo *gclue_dbus_client_interface_info (void);
guint gclue_dbus_client_override_properties (GObjectClass *klass, guint property_id_begin);


/* D-Bus method call completion functions: */
void gclue_dbus_client_complete_start (
    GClueDBusClient *object,
    GDBusMethodInvocation *invocation);

void gclue_dbus_client_complete_stop (
    GClueDBusClient *object,
    GDBusMethodInvocation *invocation);



/* D-Bus signal emissions functions: */
void gclue_dbus_client_emit_location_updated (
    GClueDBusClient *object,
    const gchar *arg_old,
    const gchar *arg_new);



/* D-Bus method calls: */
void gclue_dbus_client_call_start (
    GClueDBusClient *proxy,
    GCancellable *cancellable,
    GAsyncReadyCallback callback,
    gpointer user_data);

gboolean gclue_dbus_client_call_start_finish (
    GClueDBusClient *proxy,
    GAsyncResult *res,
    GError **error);

gboolean gclue_dbus_client_call_start_sync (
    GClueDBusClient *proxy,
    GCancellable *cancellable,
    GError **error);

void gclue_dbus_client_call_stop (
    GClueDBusClient *proxy,
    GCancellable *cancellable,
    GAsyncReadyCallback callback,
    gpointer user_data);

gboolean gclue_dbus_client_call_stop_finish (
    GClueDBusClient *proxy,
    GAsyncResult *res,
    GError **error);

gboolean gclue_dbus_client_call_stop_sync (
    GClueDBusClient *proxy,
    GCancellable *cancellable,
    GError **error);



/* D-Bus property accessors: */
const gchar *gclue_dbus_client_get_location (GClueDBusClient *object);
gchar *gclue_dbus_client_dup_location (GClueDBusClient *object);
void gclue_dbus_client_set_location (GClueDBusClient *object, const gchar *value);

guint gclue_dbus_client_get_distance_threshold (GClueDBusClient *object);
void gclue_dbus_client_set_distance_threshold (GClueDBusClient *object, guint value);

const gchar *gclue_dbus_client_get_desktop_id (GClueDBusClient *object);
gchar *gclue_dbus_client_dup_desktop_id (GClueDBusClient *object);
void gclue_dbus_client_set_desktop_id (GClueDBusClient *object, const gchar *value);

guint gclue_dbus_client_get_requested_accuracy_level (GClueDBusClient *object);
void gclue_dbus_client_set_requested_accuracy_level (GClueDBusClient *object, guint value);

gboolean gclue_dbus_client_get_active (GClueDBusClient *object);
void gclue_dbus_client_set_active (GClueDBusClient *object, gboolean value);


/* ---- */

#define GCLUE_DBUS_TYPE_CLIENT_PROXY (gclue_dbus_client_proxy_get_type ())
#define GCLUE_DBUS_CLIENT_PROXY(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), GCLUE_DBUS_TYPE_CLIENT_PROXY, GClueDBusClientProxy))
#define GCLUE_DBUS_CLIENT_PROXY_CLASS(k) (G_TYPE_CHECK_CLASS_CAST ((k), GCLUE_DBUS_TYPE_CLIENT_PROXY, GClueDBusClientProxyClass))
#define GCLUE_DBUS_CLIENT_PROXY_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GCLUE_DBUS_TYPE_CLIENT_PROXY, GClueDBusClientProxyClass))
#define GCLUE_DBUS_IS_CLIENT_PROXY(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GCLUE_DBUS_TYPE_CLIENT_PROXY))
#define GCLUE_DBUS_IS_CLIENT_PROXY_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), GCLUE_DBUS_TYPE_CLIENT_PROXY))

typedef struct _GClueDBusClientProxy GClueDBusClientProxy;
typedef struct _GClueDBusClientProxyClass GClueDBusClientProxyClass;
typedef struct _GClueDBusClientProxyPrivate GClueDBusClientProxyPrivate;

struct _GClueDBusClientProxy
{
  /*< private >*/
  GDBusProxy parent_instance;
  GClueDBusClientProxyPrivate *priv;
};

struct _GClueDBusClientProxyClass
{
  GDBusProxyClass parent_class;
};

GType gclue_dbus_client_proxy_get_type (void) G_GNUC_CONST;

void gclue_dbus_client_proxy_new (
    GDBusConnection     *connection,
    GDBusProxyFlags      flags,
    const gchar         *name,
    const gchar         *object_path,
    GCancellable        *cancellable,
    GAsyncReadyCallback  callback,
    gpointer             user_data);
GClueDBusClient *gclue_dbus_client_proxy_new_finish (
    GAsyncResult        *res,
    GError             **error);
GClueDBusClient *gclue_dbus_client_proxy_new_sync (
    GDBusConnection     *connection,
    GDBusProxyFlags      flags,
    const gchar         *name,
    const gchar         *object_path,
    GCancellable        *cancellable,
    GError             **error);

void gclue_dbus_client_proxy_new_for_bus (
    GBusType             bus_type,
    GDBusProxyFlags      flags,
    const gchar         *name,
    const gchar         *object_path,
    GCancellable        *cancellable,
    GAsyncReadyCallback  callback,
    gpointer             user_data);
GClueDBusClient *gclue_dbus_client_proxy_new_for_bus_finish (
    GAsyncResult        *res,
    GError             **error);
GClueDBusClient *gclue_dbus_client_proxy_new_for_bus_sync (
    GBusType             bus_type,
    GDBusProxyFlags      flags,
    const gchar         *name,
    const gchar         *object_path,
    GCancellable        *cancellable,
    GError             **error);


/* ---- */

#define GCLUE_DBUS_TYPE_CLIENT_SKELETON (gclue_dbus_client_skeleton_get_type ())
#define GCLUE_DBUS_CLIENT_SKELETON(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), GCLUE_DBUS_TYPE_CLIENT_SKELETON, GClueDBusClientSkeleton))
#define GCLUE_DBUS_CLIENT_SKELETON_CLASS(k) (G_TYPE_CHECK_CLASS_CAST ((k), GCLUE_DBUS_TYPE_CLIENT_SKELETON, GClueDBusClientSkeletonClass))
#define GCLUE_DBUS_CLIENT_SKELETON_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GCLUE_DBUS_TYPE_CLIENT_SKELETON, GClueDBusClientSkeletonClass))
#define GCLUE_DBUS_IS_CLIENT_SKELETON(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GCLUE_DBUS_TYPE_CLIENT_SKELETON))
#define GCLUE_DBUS_IS_CLIENT_SKELETON_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), GCLUE_DBUS_TYPE_CLIENT_SKELETON))

typedef struct _GClueDBusClientSkeleton GClueDBusClientSkeleton;
typedef struct _GClueDBusClientSkeletonClass GClueDBusClientSkeletonClass;
typedef struct _GClueDBusClientSkeletonPrivate GClueDBusClientSkeletonPrivate;

struct _GClueDBusClientSkeleton
{
  /*< private >*/
  GDBusInterfaceSkeleton parent_instance;
  GClueDBusClientSkeletonPrivate *priv;
};

struct _GClueDBusClientSkeletonClass
{
  GDBusInterfaceSkeletonClass parent_class;
};

GType gclue_dbus_client_skeleton_get_type (void) G_GNUC_CONST;

GClueDBusClient *gclue_dbus_client_skeleton_new (void);


/* ------------------------------------------------------------------------ */
/* Declarations for org.freedesktop.GeoClue2.Location */

#define GCLUE_DBUS_TYPE_LOCATION (gclue_dbus_location_get_type ())
#define GCLUE_DBUS_LOCATION(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), GCLUE_DBUS_TYPE_LOCATION, GClueDBusLocation))
#define GCLUE_DBUS_IS_LOCATION(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GCLUE_DBUS_TYPE_LOCATION))
#define GCLUE_DBUS_LOCATION_GET_IFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE ((o), GCLUE_DBUS_TYPE_LOCATION, GClueDBusLocationIface))

struct _GClueDBusLocation;
typedef struct _GClueDBusLocation GClueDBusLocation;
typedef struct _GClueDBusLocationIface GClueDBusLocationIface;

struct _GClueDBusLocationIface
{
  GTypeInterface parent_iface;

  gdouble  (*get_accuracy) (GClueDBusLocation *object);

  gdouble  (*get_altitude) (GClueDBusLocation *object);

  const gchar * (*get_description) (GClueDBusLocation *object);

  gdouble  (*get_heading) (GClueDBusLocation *object);

  gdouble  (*get_latitude) (GClueDBusLocation *object);

  gdouble  (*get_longitude) (GClueDBusLocation *object);

  gdouble  (*get_speed) (GClueDBusLocation *object);

};

GType gclue_dbus_location_get_type (void) G_GNUC_CONST;

GDBusInterfaceInfo *gclue_dbus_location_interface_info (void);
guint gclue_dbus_location_override_properties (GObjectClass *klass, guint property_id_begin);


/* D-Bus property accessors: */
gdouble gclue_dbus_location_get_latitude (GClueDBusLocation *object);
void gclue_dbus_location_set_latitude (GClueDBusLocation *object, gdouble value);

gdouble gclue_dbus_location_get_longitude (GClueDBusLocation *object);
void gclue_dbus_location_set_longitude (GClueDBusLocation *object, gdouble value);

gdouble gclue_dbus_location_get_accuracy (GClueDBusLocation *object);
void gclue_dbus_location_set_accuracy (GClueDBusLocation *object, gdouble value);

gdouble gclue_dbus_location_get_altitude (GClueDBusLocation *object);
void gclue_dbus_location_set_altitude (GClueDBusLocation *object, gdouble value);

gdouble gclue_dbus_location_get_speed (GClueDBusLocation *object);
void gclue_dbus_location_set_speed (GClueDBusLocation *object, gdouble value);

gdouble gclue_dbus_location_get_heading (GClueDBusLocation *object);
void gclue_dbus_location_set_heading (GClueDBusLocation *object, gdouble value);

const gchar *gclue_dbus_location_get_description (GClueDBusLocation *object);
gchar *gclue_dbus_location_dup_description (GClueDBusLocation *object);
void gclue_dbus_location_set_description (GClueDBusLocation *object, const gchar *value);


/* ---- */

#define GCLUE_DBUS_TYPE_LOCATION_PROXY (gclue_dbus_location_proxy_get_type ())
#define GCLUE_DBUS_LOCATION_PROXY(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), GCLUE_DBUS_TYPE_LOCATION_PROXY, GClueDBusLocationProxy))
#define GCLUE_DBUS_LOCATION_PROXY_CLASS(k) (G_TYPE_CHECK_CLASS_CAST ((k), GCLUE_DBUS_TYPE_LOCATION_PROXY, GClueDBusLocationProxyClass))
#define GCLUE_DBUS_LOCATION_PROXY_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GCLUE_DBUS_TYPE_LOCATION_PROXY, GClueDBusLocationProxyClass))
#define GCLUE_DBUS_IS_LOCATION_PROXY(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GCLUE_DBUS_TYPE_LOCATION_PROXY))
#define GCLUE_DBUS_IS_LOCATION_PROXY_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), GCLUE_DBUS_TYPE_LOCATION_PROXY))

typedef struct _GClueDBusLocationProxy GClueDBusLocationProxy;
typedef struct _GClueDBusLocationProxyClass GClueDBusLocationProxyClass;
typedef struct _GClueDBusLocationProxyPrivate GClueDBusLocationProxyPrivate;

struct _GClueDBusLocationProxy
{
  /*< private >*/
  GDBusProxy parent_instance;
  GClueDBusLocationProxyPrivate *priv;
};

struct _GClueDBusLocationProxyClass
{
  GDBusProxyClass parent_class;
};

GType gclue_dbus_location_proxy_get_type (void) G_GNUC_CONST;

void gclue_dbus_location_proxy_new (
    GDBusConnection     *connection,
    GDBusProxyFlags      flags,
    const gchar         *name,
    const gchar         *object_path,
    GCancellable        *cancellable,
    GAsyncReadyCallback  callback,
    gpointer             user_data);
GClueDBusLocation *gclue_dbus_location_proxy_new_finish (
    GAsyncResult        *res,
    GError             **error);
GClueDBusLocation *gclue_dbus_location_proxy_new_sync (
    GDBusConnection     *connection,
    GDBusProxyFlags      flags,
    const gchar         *name,
    const gchar         *object_path,
    GCancellable        *cancellable,
    GError             **error);

void gclue_dbus_location_proxy_new_for_bus (
    GBusType             bus_type,
    GDBusProxyFlags      flags,
    const gchar         *name,
    const gchar         *object_path,
    GCancellable        *cancellable,
    GAsyncReadyCallback  callback,
    gpointer             user_data);
GClueDBusLocation *gclue_dbus_location_proxy_new_for_bus_finish (
    GAsyncResult        *res,
    GError             **error);
GClueDBusLocation *gclue_dbus_location_proxy_new_for_bus_sync (
    GBusType             bus_type,
    GDBusProxyFlags      flags,
    const gchar         *name,
    const gchar         *object_path,
    GCancellable        *cancellable,
    GError             **error);


/* ---- */

#define GCLUE_DBUS_TYPE_LOCATION_SKELETON (gclue_dbus_location_skeleton_get_type ())
#define GCLUE_DBUS_LOCATION_SKELETON(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), GCLUE_DBUS_TYPE_LOCATION_SKELETON, GClueDBusLocationSkeleton))
#define GCLUE_DBUS_LOCATION_SKELETON_CLASS(k) (G_TYPE_CHECK_CLASS_CAST ((k), GCLUE_DBUS_TYPE_LOCATION_SKELETON, GClueDBusLocationSkeletonClass))
#define GCLUE_DBUS_LOCATION_SKELETON_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GCLUE_DBUS_TYPE_LOCATION_SKELETON, GClueDBusLocationSkeletonClass))
#define GCLUE_DBUS_IS_LOCATION_SKELETON(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GCLUE_DBUS_TYPE_LOCATION_SKELETON))
#define GCLUE_DBUS_IS_LOCATION_SKELETON_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), GCLUE_DBUS_TYPE_LOCATION_SKELETON))

typedef struct _GClueDBusLocationSkeleton GClueDBusLocationSkeleton;
typedef struct _GClueDBusLocationSkeletonClass GClueDBusLocationSkeletonClass;
typedef struct _GClueDBusLocationSkeletonPrivate GClueDBusLocationSkeletonPrivate;

struct _GClueDBusLocationSkeleton
{
  /*< private >*/
  GDBusInterfaceSkeleton parent_instance;
  GClueDBusLocationSkeletonPrivate *priv;
};

struct _GClueDBusLocationSkeletonClass
{
  GDBusInterfaceSkeletonClass parent_class;
};

GType gclue_dbus_location_skeleton_get_type (void) G_GNUC_CONST;

GClueDBusLocation *gclue_dbus_location_skeleton_new (void);


G_END_DECLS

#endif /* __GEOCLUE_INTERFACE_H__ */
