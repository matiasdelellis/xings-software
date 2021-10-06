/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2021 Matias De lellis <mati86dl@gmail.com>
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

#include "gpk-session.h"
#include "gpk-gnome-proxy.h"

static void     gpk_gnome_proxy_finalize   (GObject		*object);

#define GPK_GNOME_SESSION_MANAGER_SERVICE		"org.gnome.SessionManager"
#define GPK_GNOME_SESSION_MANAGER_PATH			"/org/gnome/SessionManager"
#define GPK_GNOME_SESSION_MANAGER_INTERFACE		"org.gnome.SessionManager"
#define GPK_GNOME_SESSION_LOGOUT_METHOD			"Logout"
#define GPK_GNOME_SESSION_LOGOUT_ARGUMENTS		g_variant_new ("(u)", 1)
#define GPK_GNOME_SESSION_REBOOT_METHOD			"Reboot"
#define GPK_GNOME_SESSION_REBOOT_ARGUMENTS		NULL

typedef struct
{
	GDBusConnection *connection;
	GDBusProxy	*proxy;
} GpkGnomeProxyPrivate;


G_DEFINE_TYPE_WITH_PRIVATE (GpkGnomeProxy, gpk_gnome_proxy, G_TYPE_OBJECT)

/**
 * gpk_gnome_proxy_logout:
 **/
gboolean
gpk_gnome_proxy_logout (GpkGnomeProxy *proxy, GError **error)
{
	GpkGnomeProxyPrivate *priv;
	gboolean ret = FALSE;
	GVariant *res;

	g_return_val_if_fail (GPK_IS_GNOME_PROXY (proxy), FALSE);

	priv = gpk_gnome_proxy_get_instance_private (GPK_GNOME_PROXY (proxy));

	res = g_dbus_proxy_call_sync (priv->proxy,
	                              GPK_GNOME_SESSION_LOGOUT_METHOD,
	                              GPK_GNOME_SESSION_LOGOUT_ARGUMENTS,
	                              G_DBUS_CALL_FLAGS_NONE,
	                              G_MAXINT,
	                              NULL,
	                              error);

	if (res) {
		g_variant_unref (res);
		ret = TRUE;
	}

	return ret;
}

/**
 * gpk_gnome_proxy_reboot:
 **/
gboolean
gpk_gnome_proxy_reboot (GpkGnomeProxy *proxy, GError **error)
{
	GpkGnomeProxyPrivate *priv;
	gboolean ret = FALSE;
	GVariant *res;

	g_return_val_if_fail (GPK_IS_GNOME_PROXY (proxy), FALSE);

	priv = gpk_gnome_proxy_get_instance_private (GPK_GNOME_PROXY (proxy));

	res = g_dbus_proxy_call_sync (priv->proxy,
	                              GPK_GNOME_SESSION_REBOOT_METHOD,
	                              GPK_GNOME_SESSION_REBOOT_ARGUMENTS,
	                              G_DBUS_CALL_FLAGS_NONE,
	                              G_MAXINT,
	                              NULL,
	                              error);

	if (res) {
		g_variant_unref (res);
		ret = TRUE;
	}

	return ret;
}

/**
 * gpk_gnome_proxy_is_conected:
 **/
gboolean
gpk_gnome_proxy_is_conected (GpkGnomeProxy *proxy)
{
	GpkGnomeProxyPrivate *priv;
	g_return_val_if_fail (GPK_IS_GNOME_PROXY (proxy), FALSE);
	priv = gpk_gnome_proxy_get_instance_private (GPK_GNOME_PROXY (proxy));
	return (priv->proxy != NULL);
}

/**
 * gpk_gnome_proxy_class_init:
 * @klass: This class instance
 **/
static void
gpk_gnome_proxy_class_init (GpkGnomeProxyClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gpk_gnome_proxy_finalize;
}

/**
 * gpk_gnome_proxy_init:
 * @proxy: This class instance
 **/
static void
gpk_gnome_proxy_init (GpkGnomeProxy *proxy)
{
	GpkGnomeProxyPrivate *priv;
	GError *error = NULL;

	priv = gpk_gnome_proxy_get_instance_private (GPK_GNOME_PROXY (proxy));

	priv->connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
	if (priv->connection == NULL) {
		g_message ("Failed to connect to the session bus: %s", error->message);
		g_error_free (error);
		return;
	}

	priv->proxy = gpk_session_get_proxy_if_service_present (priv->connection,
	                                                        G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START |
	                                                        G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES |
	                                                        G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS,
	                                                        GPK_GNOME_SESSION_MANAGER_SERVICE,
	                                                        GPK_GNOME_SESSION_MANAGER_PATH,
	                                                        GPK_GNOME_SESSION_MANAGER_INTERFACE,
	                                                        &error);

	if (error) {
		g_clear_error (&error);
	}
}

/**
 * gpk_gnome_proxy_finalize:
 * @object: This class instance
 **/
static void
gpk_gnome_proxy_finalize (GObject *object)
{
	GpkGnomeProxyPrivate *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GPK_IS_GNOME_PROXY (object));

	priv = gpk_gnome_proxy_get_instance_private (GPK_GNOME_PROXY (object));

	if (priv->proxy)
		g_clear_object (&priv->proxy);
	if (priv->connection)
		g_object_unref (priv->connection);

	G_OBJECT_CLASS (gpk_gnome_proxy_parent_class)->finalize (object);
}

/**
 * gpk_gnome_proxy_new:
 * Return value: new GpkGnomeProxy instance.
 **/
GpkGnomeProxy *
gpk_gnome_proxy_new (void)
{
	GpkGnomeProxy *proxy;
	proxy = g_object_new (GPK_TYPE_GNOME_PROXY, NULL);
	return proxy;
}
