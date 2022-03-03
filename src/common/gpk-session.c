/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008-2009 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <glib.h>
#include <gio/gio.h>

#include "gpk-gnome-proxy.h"
#include "gpk-xfce-proxy.h"

#ifdef HAVE_SYSTEMD
#include "systemd-proxy.h"
#endif

#include "gpk-session.h"

static void     gpk_session_finalize   (GObject		*object);

static gpointer gpk_session_object = NULL;

typedef struct
{
	GpkGnomeProxy	*gnome_proxy;
	GpkXfceProxy	*xfce_proxy;
#ifdef HAVE_SYSTEMD
	SystemdProxy	*systemd_proxy;
#endif
} GpkSessionPrivate;


G_DEFINE_TYPE_WITH_PRIVATE (GpkSession, gpk_session, G_TYPE_OBJECT)


/**
 * gpk_session_logout:
 **/
gboolean
gpk_session_reboot (GpkSession *session, GError **error)
{
	gboolean ret = FALSE;

	GpkSessionPrivate *priv;

	g_return_val_if_fail (GPK_IS_SESSION (session), FALSE);

	priv = gpk_session_get_instance_private (GPK_SESSION (session));

	if (gpk_gnome_proxy_is_conected (priv->gnome_proxy)) {
		ret = gpk_gnome_proxy_reboot (priv->gnome_proxy, error);
	} else if (gpk_xfce_proxy_is_conected (priv->xfce_proxy)) {
		ret = gpk_xfce_proxy_reboot (priv->xfce_proxy, error);
	} else {
#ifdef HAVE_SYSTEMD
		ret = systemd_proxy_restart (priv->systemd_proxy, error);
#endif
	}

	return ret;
}

gboolean
gpk_session_can_reboot (GpkSession  *session,
                        gboolean    *can_reboot,
                        GError     **error)
{
#ifdef HAVE_SYSTEMD
	GpkSessionPrivate *priv;
#endif
	g_return_val_if_fail (GPK_IS_SESSION (session), FALSE);
#ifdef HAVE_SYSTEMD
	priv = gpk_session_get_instance_private (GPK_SESSION (session));
	return systemd_proxy_can_restart (priv->systemd_proxy, can_reboot, error);
#else
	// TODO: Get from session manager
	*can_reboot = TRUE;
	return TRUE;
#endif
}

/**
 * gpk_session_logout:
 **/
gboolean
gpk_session_logout (GpkSession *session, GError **error)
{
	gboolean ret = FALSE;

	GpkSessionPrivate *priv;

	g_return_val_if_fail (GPK_IS_SESSION (session), FALSE);

	priv = gpk_session_get_instance_private (GPK_SESSION (session));

	if (gpk_gnome_proxy_is_conected (priv->gnome_proxy)) {
		ret = gpk_gnome_proxy_logout (priv->gnome_proxy, error);
	}

	if (gpk_xfce_proxy_is_conected (priv->xfce_proxy)) {
		ret = gpk_xfce_proxy_logout (priv->xfce_proxy, error);
	}
	return ret;
}


/**
 * gpk_session_get_proxy_if_service_present:
 *
 **/
GDBusProxy *
gpk_session_get_proxy_if_service_present (GDBusConnection *connection,
                                          GDBusProxyFlags  flags,
                                          const char      *bus_name,
                                          const char      *object_path,
                                          const char      *interface,
                                          GError         **error)
{
	GDBusProxy *proxy;
	char *owner;

	proxy = g_dbus_proxy_new_sync (connection,
	                               flags,
	                               NULL,
	                               bus_name,
	                               object_path,
	                               interface,
	                               NULL,
	                               error);

	if (!proxy)
		return NULL;

	/* is there anyone actually providing the service? */
	owner = g_dbus_proxy_get_name_owner (proxy);
	if (owner == NULL) {
		g_clear_object (&proxy);
		g_set_error (error, G_DBUS_ERROR, G_DBUS_ERROR_NAME_HAS_NO_OWNER,
		             "The name %s is not owned", bus_name);
	}
	else
		g_free (owner);

	return proxy;
}

/**
 * gpk_session_class_init:
 * @klass: This class instance
 **/
static void
gpk_session_class_init (GpkSessionClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gpk_session_finalize;
}

/**
 * gpk_session_init:
 * @session: This class instance
 **/
static void
gpk_session_init (GpkSession *session)
{
	GpkSessionPrivate *priv;

	priv = gpk_session_get_instance_private (GPK_SESSION (session));

	priv->gnome_proxy = gpk_gnome_proxy_new ();
	priv->xfce_proxy = gpk_xfce_proxy_new ();
#ifdef HAVE_SYSTEMD
	priv->systemd_proxy = systemd_proxy_new ();
#endif
}

/**
 * gpk_session_finalize:
 * @object: This class instance
 **/
static void
gpk_session_finalize (GObject *object)
{
	GpkSessionPrivate *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GPK_IS_SESSION (object));

	priv = gpk_session_get_instance_private (GPK_SESSION (object));

	if (priv->gnome_proxy)
		g_clear_object (&priv->gnome_proxy);
	if (priv->xfce_proxy)
		g_clear_object (&priv->xfce_proxy);

#ifdef HAVE_SYSTEMD
	if (priv->systemd_proxy) {
		systemd_proxy_free (priv->systemd_proxy);
		priv->systemd_proxy = NULL;
	}
#endif

	G_OBJECT_CLASS (gpk_session_parent_class)->finalize (object);
}

/**
 * gpk_session_new:
 * Return value: new GpkSession instance.
 **/
GpkSession *
gpk_session_new (void)
{
	if (gpk_session_object != NULL) {
		g_object_ref (gpk_session_object);
	} else {
		gpk_session_object = g_object_new (GPK_TYPE_SESSION, NULL);
		g_object_add_weak_pointer (gpk_session_object, &gpk_session_object);
	}
	return GPK_SESSION (gpk_session_object);
}
