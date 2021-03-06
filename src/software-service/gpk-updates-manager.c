/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2021 Matias De lellis <mati86dl@gmail.com>
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "config.h"

#include <common/gpk-common.h>
#include <common/gpk-session.h>

#include "gpk-updates-notification.h"

#include "gpk-updates-checker.h"
#include "gpk-updates-download.h"
#include "gpk-updates-refresh.h"
#include "gpk-updates-shared.h"

#include "gpk-updates-manager.h"

/* We do not depend on UPOWER, but we must understand this enum */
typedef enum {
	UP_DEVICE_LEVEL_UNKNOWN,
	UP_DEVICE_LEVEL_NONE,
	UP_DEVICE_LEVEL_DISCHARGING,
	UP_DEVICE_LEVEL_LOW,
	UP_DEVICE_LEVEL_CRITICAL,
	UP_DEVICE_LEVEL_ACTION,
	UP_DEVICE_LEVEL_LAST
} UpDeviceLevel;

struct _GpkUpdatesManager
{
	GObject			_parent;

	GpkUpdatesShared	*shared;

	guint			 check_startup_id;	/* 60s after startup */
	guint			 check_hourly_id;	/* and then every hour */

	GDBusProxy		*proxy_upower;
	GNetworkMonitor		*network_monitor;

	guint			 dbus_watch_id;

	GpkUpdatesRefresh	*refresh;
	GpkUpdatesChecker	*checker;
	GpkUpdatesDownload	*download;

	GpkUpdatesNotification	*notification;
};

G_DEFINE_TYPE (GpkUpdatesManager, gpk_updates_manager, G_TYPE_OBJECT)


/**
 *  Common definitions.
 */

#define SECONDS_IN_AN_HOUR (60 * 60)


/**
 *  Utils
 */
static gboolean
gpk_updates_manager_is_online (GpkUpdatesManager *manager)
{
	if (!g_network_monitor_get_network_available (manager->network_monitor))
		return FALSE;

	if (g_network_monitor_get_network_metered (manager->network_monitor) &&
	    !g_settings_get_boolean (gpk_updates_shared_get_settings (manager->shared),
	                             GPK_SETTINGS_UPDATE_ON_MOBILE))
		return FALSE;

	return TRUE;
}

UpDeviceLevel
gpk_updates_manager_get_battery_status (GpkUpdatesManager *manager)
{
	GVariant *val = NULL;
	UpDeviceLevel level = UP_DEVICE_LEVEL_UNKNOWN;

	if (!manager->proxy_upower) {
		g_debug ("no UPower support, so not doing power level checks");
		level = UP_DEVICE_LEVEL_NONE;
		goto out;
	}

	val = g_dbus_proxy_get_cached_property (manager->proxy_upower, "WarningLevel");
	if (!val) {
		g_debug ("no UPower support, so not doing power level checks");
		level = UP_DEVICE_LEVEL_NONE;
		goto out;
	}
	level = g_variant_get_uint32 (val);
	g_variant_unref (val);

out:
	return level;
}

static void
gpk_updates_notififier_launch_update_viewer (GpkUpdatesManager *manager)
{
	gboolean ret;
	GError *error = NULL;

	ret = g_spawn_command_line_async (BINDIR "/xings-software-update",
	                                  &error);

	if (!ret) {
		g_warning ("Failure launching update viewer: %s", error->message);
		g_error_free (error);
	}
}


/**
 *  Logic for update scheduler.
 */

static void
gpk_updates_manager_notify_updates (GpkUpdatesManager *manager, gboolean downloaded)
{
	guint updates_count = 0, important_packages = 0;

	updates_count = gpk_updates_checker_get_updates_count (manager->checker);
	important_packages = gpk_updates_checker_get_important_updates_count (manager->checker);

	gpk_updates_notification_should_notify_updates (manager->notification, downloaded, updates_count, important_packages);
}

static void
gpk_updates_manager_auto_download_done (GpkUpdatesManager *manager)
{
	g_debug ("Download done.");
	gpk_updates_manager_notify_updates (manager, TRUE);
}

static void
gpk_updates_manager_checker_has_updates (GpkUpdatesManager *manager)
{
	UpDeviceLevel battery_level = UP_DEVICE_LEVEL_UNKNOWN;
	gboolean auto_download = FALSE;
	gchar **package_ids;

	battery_level = gpk_updates_manager_get_battery_status (manager);
	auto_download = g_settings_get_boolean (gpk_updates_shared_get_settings (manager->shared),
	                                        GPK_SETTINGS_AUTO_DOWNLOAD_UPDATES);

	/* should we auto-download the updates? */
	if (auto_download && battery_level >= UP_DEVICE_LEVEL_DISCHARGING) {
		g_debug ("there are updates to download");

		package_ids = gpk_updates_checker_get_update_packages_ids (manager->checker);
		gpk_updates_download_auto_download_updates (manager->download, package_ids);
		g_strfreev (package_ids);
	}
	else {
		g_debug ("there are updates to notify");
		gpk_updates_manager_notify_updates (manager, FALSE);
	}
}

static void
gpk_updates_manager_refresh_cache_done (GpkUpdatesManager *manager)
{
	gpk_updates_checker_check_for_updates (manager->checker);
}

static void
gpk_updates_manager_generic_error (GpkUpdatesManager *manager)
{
	/* TODO: Do something non-generic. */
	g_debug ("generic error");
}

static void
gpk_updates_manager_check_updates (GpkUpdatesManager *manager)
{
	/* never refresh when the battery is low */
	if (gpk_updates_manager_get_battery_status (manager) >= UP_DEVICE_LEVEL_LOW) {
		g_debug ("not getting updates on low power");
		return;
	}

	/* never check for updates when offline */
	if (!gpk_updates_manager_is_online(manager)) {
		g_debug ("coul not getting updates due offline");
		return;
	}

	/* check if need refresh cache. */
	gpk_updates_refresh_update_cache (manager->refresh);
}

static gboolean
gpk_updates_manager_check_hourly_cb (gpointer data)
{
	GpkUpdatesManager *manager = GPK_UPDATES_MANAGER (data);

	g_debug ("Hourly updates check");
	gpk_updates_manager_check_updates (manager);

	return G_SOURCE_CONTINUE;
}

static void
gpk_updates_manager_stop_updates_check (GpkUpdatesManager *manager)
{
	if (manager->check_hourly_id == 0)
		return;

	g_source_remove (manager->check_hourly_id);
	manager->check_hourly_id = 0;
}

static void
gpk_updates_manager_restart_updates_check (GpkUpdatesManager *manager)
{
	gpk_updates_manager_stop_updates_check (manager);
	gpk_updates_manager_check_updates (manager);

	manager->check_hourly_id
		= g_timeout_add_seconds (SECONDS_IN_AN_HOUR,
		                         gpk_updates_manager_check_hourly_cb,
		                         manager);
}

static gboolean
gpk_updates_manager_check_updates_on_startup_cb (gpointer user_data)
{
	GpkUpdatesManager *manager = GPK_UPDATES_MANAGER (user_data);

	g_return_val_if_fail (GPK_IS_UPDATES_MANAGER (manager), G_SOURCE_REMOVE);

	g_debug ("First hourly updates check");
	gpk_updates_manager_restart_updates_check (manager);

	manager->check_startup_id = 0;

	return G_SOURCE_REMOVE;
}


/**
 * React when updater is launched or closed.
 */

static void
gpk_updates_viewer_appeared_cb (GDBusConnection *connection,
                                const gchar     *name,
                                const gchar     *name_owner,
                                gpointer         user_data)
{
}

static void
gpk_updates_viewer_vanished_cb (GDBusConnection *connection,
                                const gchar     *name,
                                gpointer         user_data)
{
	g_debug ("Xings package updates dialog is closed");
}


/*
 * Signals on user actions.
 */

static void
gpk_updates_manager_notification_show_update_viewer_cb (GpkUpdatesNotification *notification,
                                                        GpkUpdatesManager      *manager)
{
	gpk_updates_notififier_launch_update_viewer (manager);
}

static void
gpk_updates_manager_notification_ignore_updates_cb (GpkUpdatesNotification *notification,
                                                    GpkUpdatesManager      *manager)
{
	g_debug ("User just ignore updates from notification...");
}

static void
gpk_updates_manager_notification_reboot_system_cb (GpkUpdatesNotification *notification,
                                                   GpkUpdatesManager      *manager)
{
	GpkSession *session = gpk_session_new ();
	gpk_session_reboot (session, NULL);
	g_object_unref (session);
}

/**
 *  GpkUpdatesManager:
 */

static void
gpk_updates_manager_dispose (GObject *object)
{
	GpkUpdatesManager *manager;

	manager = GPK_UPDATES_MANAGER (object);

	g_debug ("Stopping updates manager");

	gpk_updates_manager_stop_updates_check (manager);

	if (manager->check_startup_id != 0) {
		g_source_remove (manager->check_startup_id);
		manager->check_startup_id = 0;
	}

	if (manager->dbus_watch_id > 0) {
		g_bus_unwatch_name (manager->dbus_watch_id);
		manager->dbus_watch_id = 0;
	}

	g_clear_object (&manager->proxy_upower);

	g_clear_object (&manager->notification);

	g_clear_object (&manager->refresh);
	g_clear_object (&manager->checker);
	g_clear_object (&manager->shared);
	g_clear_object (&manager->download);

	g_debug ("Stopped updates manager");

	G_OBJECT_CLASS (gpk_updates_manager_parent_class)->dispose (object);
}

static void
gpk_updates_manager_class_init (GpkUpdatesManagerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gpk_updates_manager_dispose;
}

static void
gpk_updates_manager_init (GpkUpdatesManager *manager)
{
	GError *error = NULL;

	g_debug ("Starting updates manager");

	/* The shared code between the different tasks */

	manager->shared = gpk_updates_shared_get ();

	/* the cache manager.*/

	manager->refresh = gpk_updates_refresh_new ();
	g_signal_connect_swapped (manager->refresh, "valid-cache",
	                          G_CALLBACK (gpk_updates_manager_refresh_cache_done), manager);
	g_signal_connect_swapped (manager->refresh, "error-refresh",
	                          G_CALLBACK (gpk_updates_manager_generic_error), manager);

	/* The check for updates task */

	manager->checker = gpk_updates_checker_new ();
	g_signal_connect_swapped (manager->checker, "has-updates",
	                          G_CALLBACK (gpk_updates_manager_checker_has_updates), manager);
	g_signal_connect_swapped (manager->checker, "error-checking",
	                          G_CALLBACK (gpk_updates_manager_generic_error), manager);

	/* the update download task */

	manager->download = gpk_updates_download_new ();
	g_signal_connect_swapped (manager->download, "download-done",
	                          G_CALLBACK (gpk_updates_manager_auto_download_done), manager);
	g_signal_connect_swapped (manager->download, "error-downloading",
	                          G_CALLBACK (gpk_updates_manager_generic_error), manager);

	/* the notification manager */

	manager->notification = gpk_updates_notification_new ();
	g_signal_connect (manager->notification, "show-update-viewer",
	                  G_CALLBACK (gpk_updates_manager_notification_show_update_viewer_cb), manager);
	g_signal_connect (manager->notification, "ignore-updates",
	                  G_CALLBACK (gpk_updates_manager_notification_ignore_updates_cb), manager);
	g_signal_connect (manager->notification, "reboot-system",
	                  G_CALLBACK (gpk_updates_manager_notification_reboot_system_cb), manager);

	/* we have to consider the network connection before looking for updates */

	manager->network_monitor = g_network_monitor_get_default ();

	/* connect to UPower to get the system power state */
	manager->proxy_upower =
		g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
		                               G_DBUS_PROXY_FLAGS_NONE,
		                               NULL,
		                               "org.freedesktop.UPower",
		                               "/org/freedesktop/UPower/devices/DisplayDevice",
		                               "org.freedesktop.UPower.Device",
		                               NULL,
		                               &error);
	if (!manager->proxy_upower) {
		g_warning ("failed to connect to upower: %s", error->message);
		g_error_free (error);
	}

	/* do a first check 60 seconds after login, and then every hour */
	manager->check_startup_id =
		g_timeout_add_seconds (60,
		                       gpk_updates_manager_check_updates_on_startup_cb,
		                       manager);
	g_source_set_name_by_id (manager->check_startup_id,
	                         "[GpkUpdatesManager] periodic check");

	/* Check if starts the update viewer to hide the icon. */
	manager->dbus_watch_id = g_bus_watch_name (G_BUS_TYPE_SESSION,
	                                            "org.xings.SoftwareUpdate",
	                                            G_BUS_NAME_WATCHER_FLAGS_NONE,
	                                            gpk_updates_viewer_appeared_cb,
	                                            gpk_updates_viewer_vanished_cb,
	                                            manager,
	                                            NULL);

	/* success */
	g_debug ("Started updates manager");
}

GpkUpdatesManager *
gpk_updates_manager_new (void)
{
	GpkUpdatesManager *manager;
	manager = g_object_new (GPK_TYPE_UPDATES_MANAGER, NULL);
	return GPK_UPDATES_MANAGER (manager);
}

