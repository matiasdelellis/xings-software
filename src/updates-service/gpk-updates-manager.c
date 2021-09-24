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

#include <glib/gi18n.h>
#include <packagekit-glib2/packagekit.h>

#include <common/gpk-common.h>

#include "gpk-updates-applet.h"
#include "gpk-updates-notification.h"

#include "gpk-updates-checker.h"
#include "gpk-updates-download.h"
#include "gpk-updates-refresh.h"
#include "gpk-updates-shared.h"

#include "gpk-updates-manager.h"


struct _GpkUpdatesManager
{
	GObject			_parent;

	GpkUpdatesShared	*shared;

	guint			 check_startup_id;	/* 60s after startup */
	guint			 check_hourly_id;	/* and then every hour */

	GNetworkMonitor		*network_monitor;

	guint			 dbus_watch_id;

	GpkUpdatesRefresh	*refresh;
	GpkUpdatesChecker	*checker;
	GpkUpdatesDownload	*download;

	GpkUpdatesApplet	*applet;
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

	if (g_network_monitor_get_network_metered (manager->network_monitor))
		return FALSE;

	return TRUE;
}

static void
gpk_updates_notififier_launch_update_viewer (GpkUpdatesManager *manager)
{
	gboolean ret;
	GError *error = NULL;

	ret = g_spawn_command_line_async (BINDIR "/xings-package-updates",
	                                  &error);

	if (!ret) {
		g_warning ("Failure launching update viewer: %s", error->message);
		g_error_free (error);
	}
}

static void
gpk_updates_manager_should_notify_for_importance (GpkUpdatesManager *manager)
{
	guint updates_count = 0, important_packages = 0;

	important_packages = gpk_updates_checker_get_important_updates_count (manager->checker);

	if (important_packages) {
		gpk_updates_notification_show_critical_updates (manager->notification,
		                                                important_packages);
		gpk_updates_applet_show_critical_updates (manager->applet,
		                                          important_packages);
	} else {
		updates_count = gpk_updates_checker_get_updates_count (manager->checker);
		gpk_updates_notification_maybe_show_normal_updates (manager->notification,
		                                                    updates_count);
		gpk_updates_applet_show_normal_updates (manager->applet,
		                                        updates_count);
	}
}


/**
 *  Logic for update scheduler.
 */

static void
gpk_updates_manager_auto_download_done (GpkUpdatesManager *manager)
{
	g_debug ("Download done.");
}

static void
gpk_updates_manager_checker_has_updates (GpkUpdatesManager *manager)
{
	gchar **package_ids;

	/* should we auto-download the updates? */
	if (g_settings_get_boolean (gpk_updates_shared_get_settings (manager->shared),
	                            GPK_SETTINGS_AUTO_DOWNLOAD_UPDATES)) {
		g_debug ("there are updates to download");

		package_ids = gpk_updates_checker_get_update_packages_ids (manager->checker);
		gpk_updates_download_auto_download_updates (manager->download, package_ids);
		g_strfreev (package_ids);
	}
	else {
		g_debug ("there are updates to notify");
		gpk_updates_manager_should_notify_for_importance (manager);
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
	/* never check for updates when offline */
	if (!gpk_updates_manager_is_online(manager))
		return;

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
	GpkUpdatesManager *manager = GPK_UPDATES_MANAGER(user_data);

	g_debug ("Xings package updates appeared on dbus. Hiding applet.");

	gpk_updates_applet_hide (manager->applet);
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
gpk_updates_manager_applet_activated_cb (GpkUpdatesApplet  *applet,
                                         GpkUpdatesManager *manager)
{
	gpk_updates_notififier_launch_update_viewer (manager);
}

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

	g_clear_object (&manager->applet);
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
	g_debug ("Starting updates manager");

	/* The shared code between the different tasks */
	manager->shared = gpk_updates_shared_get ();

	/* the notification applet */
	manager->applet = gpk_updates_applet_new ();

	/* the notification manager */
	manager->notification = gpk_updates_notification_new ();

	/* we have to consider the network connection before looking for updates */
	manager->network_monitor = g_network_monitor_get_default ();

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

	/* do a first check 60 seconds after login, and then every hour */
	manager->check_startup_id =
		g_timeout_add_seconds (60,
		                       gpk_updates_manager_check_updates_on_startup_cb,
		                       manager);
	g_source_set_name_by_id (manager->check_startup_id,
	                         "[GpkUpdatesManager] periodic check");

	/* Check if starts the update viewer to hide the icon. */
	manager->dbus_watch_id = g_bus_watch_name (G_BUS_TYPE_SESSION,
	                                            "org.xings.PackageUpdates",
	                                            G_BUS_NAME_WATCHER_FLAGS_NONE,
	                                            gpk_updates_viewer_appeared_cb,
	                                            gpk_updates_viewer_vanished_cb,
	                                            manager,
	                                            NULL);

	/* show update viewer on user actions */

	g_signal_connect (manager->applet, "activate",
			  G_CALLBACK (gpk_updates_manager_applet_activated_cb), manager);

	g_signal_connect (manager->notification, "show-update-viewer",
			  G_CALLBACK (gpk_updates_manager_notification_show_update_viewer_cb), manager);
	g_signal_connect (manager->notification, "ignore-updates",
			  G_CALLBACK (gpk_updates_manager_notification_ignore_updates_cb), manager);

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


