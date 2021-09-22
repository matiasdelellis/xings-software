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

#include "gpk-common.h"
#include "gpk-updates-applet.h"
#include "gpk-updates-notification.h"

#include "gpk-updates-notifier.h"


struct _GpkUpdatesNotifier
{
	GObject			_parent;

	PkControl		*control;
	PkTask			*task;
	GCancellable		*cancellable;

	GSettings		*settings;

	guint			 check_startup_id;	/* 60s after startup */
	guint			 check_hourly_id;	/* and then every hour */

	GNetworkMonitor		*network_monitor;

	GPtrArray		*update_packages;

	guint			 dbus_watch_id;

	GpkUpdatesApplet	*applet;
	GpkUpdatesNotification	*notification;
};

G_DEFINE_TYPE (GpkUpdatesNotifier, gpk_updates_notifier, G_TYPE_OBJECT)


/**
 *  Common definitions.
 */

#define SECONDS_IN_AN_HOUR (60 * 60)


/**
 *  Utils
 */

static gboolean
gpk_updates_notifier_is_online (GpkUpdatesNotifier *notifier)
{
	if (!g_network_monitor_get_network_available (notifier->network_monitor))
		return FALSE;

	if (g_network_monitor_get_network_metered (notifier->network_monitor))
		return FALSE;

	return TRUE;
}

static void
gpk_updates_notififier_launch_update_viewer (GpkUpdatesNotifier *notifier)
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
gpk_updates_notifier_should_notify_for_importance (GpkUpdatesNotifier *notifier)
{
	PkPackage *pkg;
	guint i, important_packages = 0;

	/* check each package */
	for (i = 0; i < notifier->update_packages->len; i++) {
		pkg = g_ptr_array_index (notifier->update_packages, i);
		if (pk_package_get_info (pkg) == PK_INFO_ENUM_SECURITY ||
		    pk_package_get_info (pkg) == PK_INFO_ENUM_IMPORTANT)
			important_packages++;
	}

	if (important_packages > 0) {
		gpk_updates_notification_show_critical_updates (notifier->notification,
		                                                important_packages);
		gpk_updates_applet_show_critical_updates (notifier->applet,
		                                          important_packages);
	} else {
		gpk_updates_notification_maybe_show_normal_updates (notifier->notification,
		                                                    notifier->update_packages->len);
		gpk_updates_applet_show_normal_updates (notifier->applet,
		                                        notifier->update_packages->len);
	}
}


/*
 * Search for updates.
 */

static void
gpk_updates_notifier_pk_check_updates_finished_cb (GObject            *object,
                                                   GAsyncResult       *res,
                                                   GpkUpdatesNotifier *notifier)
{
	PkResults *results;
	GError *error = NULL;
	PkError *error_code = NULL;

	PkClient *client = PK_CLIENT(object);

	/* get the results */
	results = pk_client_generic_finish (PK_CLIENT(client), res, &error);
	if (results == NULL) {
		if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
			g_error_free (error);
			return;
		}
		g_warning ("failed to get updates: %s", error->message);
		g_error_free (error);
		gpk_updates_notification_show_failed (notifier->notification);
		gpk_updates_applet_show_failed (notifier->applet);
		goto out;
	}

	/* check error code */
	error_code = pk_results_get_error_code (results);
	if (error_code != NULL) {
		g_warning ("failed to get updates: %s, %s",
		           pk_error_enum_to_string (pk_error_get_code (error_code)),
		           pk_error_get_details (error_code));
		switch (pk_error_get_code (error_code)) {
			case PK_ERROR_ENUM_CANCELLED_PRIORITY:
			case PK_ERROR_ENUM_TRANSACTION_CANCELLED:
				g_debug ("ignoring error");
				break;
			default:
				gpk_updates_notification_show_failed (notifier->notification);
				gpk_updates_applet_show_failed (notifier->applet);
				break;
		}
		goto out;
	}

	/* so we can download or check for important & security updates */
	if (notifier->update_packages != NULL)
		g_ptr_array_unref (notifier->update_packages);
	notifier->update_packages = pk_results_get_package_array (results);

	/* we have no updates */
	if (notifier->update_packages->len == 0) {
		g_debug ("no updates");
		goto out;
	}

	/* just check to see if should notify */
	gpk_updates_notifier_should_notify_for_importance (notifier);

out:
	if (error_code != NULL)
		g_object_unref (error_code);
	if (results != NULL)
		g_object_unref (results);
}

static void
gpk_updates_notifier_pk_check_updates (GpkUpdatesNotifier *notifier)
{
	/* optimize the amount of downloaded data by setting the cache age */
	pk_client_set_cache_age (PK_CLIENT(notifier->task),
	                         g_settings_get_int (notifier->settings,
	                                             GPK_SETTINGS_FREQUENCY_GET_UPDATES));

	/* get new update list */
	pk_client_get_updates_async (PK_CLIENT(notifier->task),
	                             pk_bitfield_value (PK_FILTER_ENUM_NONE),
	                             notifier->cancellable,
	                             NULL, NULL,
	                             (GAsyncReadyCallback) gpk_updates_notifier_pk_check_updates_finished_cb,
	                             notifier);
}


/*
 * Refresh cache.
 */

static void
gpk_updates_notifier_pk_refresh_cache_finished_cb (GObject            *object,
                                                   GAsyncResult       *res,
                                                   GpkUpdatesNotifier *notifier)
{
	PkResults *results;
	GError *error = NULL;
	PkError *error_code = NULL;

	PkClient *client = PK_CLIENT(object);

	/* get the results */
	results = pk_client_generic_finish (PK_CLIENT(client), res, &error);
	if (results == NULL) {
		if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
			g_error_free (error);
			return;
		}
		g_warning ("failed to refresh the cache: %s", error->message);
		g_error_free (error);
		return;
	}

	/* check error code */
	error_code = pk_results_get_error_code (results);
	if (error_code != NULL) {
		g_warning ("failed to refresh the cache: %s, %s",
		           pk_error_enum_to_string (pk_error_get_code (error_code)),
		           pk_error_get_details (error_code));
		g_object_unref (error_code);
		g_object_unref (results);
	}

	g_debug ("Cache was updated. Looking for updates.");
	gpk_updates_notifier_pk_check_updates (notifier);
}

static void
gpk_updates_notifier_pk_refresh_cache (GpkUpdatesNotifier *notifier)
{
	/* optimize the amount of downloaded data by setting the cache age */
	pk_client_set_cache_age (PK_CLIENT(notifier->task),
	                         g_settings_get_int (notifier->settings,
	                                             GPK_SETTINGS_FREQUENCY_REFRESH_CACHE));

	pk_client_refresh_cache_async (PK_CLIENT(notifier->task),
	                               TRUE,
	                               notifier->cancellable,
	                               NULL, NULL,
	                               (GAsyncReadyCallback) gpk_updates_notifier_pk_refresh_cache_finished_cb,
	                               notifier);
}


/*
 * Scheduler the refesh cache to optimize usage.
 */

static void
gpk_updates_notifier_pk_get_time_since_refresh_cache_cb (GObject            *object,
                                                         GAsyncResult       *res,
                                                         GpkUpdatesNotifier *notifier)
{
	GError *error = NULL;
	guint seconds;
	guint threshold;

	PkControl *control = PK_CONTROL (object);

	/* get the result */
	seconds = pk_control_get_time_since_action_finish (control, res, &error);
	if (seconds == 0) {
		g_warning ("failed to get time: %s", error->message);
		g_error_free (error);
		return;
	}

	/* have we passed the timeout? */
	threshold = g_settings_get_int (notifier->settings,
	                                GPK_SETTINGS_FREQUENCY_GET_UPDATES);

	if (seconds < threshold) {
		g_debug ("not refresh before timeout, thresh=%u, now=%u", threshold, seconds);
		gpk_updates_notifier_pk_check_updates (notifier);
		return;
	}

	/* send signal */
	g_debug ("must refresh cache");
	gpk_updates_notifier_pk_refresh_cache (notifier);
}

static void
gpk_updates_notifier_pk_check_refresh_cache (GpkUpdatesNotifier *notifier)
{
	guint threshold;

	g_return_if_fail (GPK_IS_UPDATES_NOTIFIER (notifier));

	/* if we don't want to auto check for updates, don't do this either */
	threshold = g_settings_get_int (notifier->settings,
	                                GPK_SETTINGS_FREQUENCY_GET_UPDATES);
	if (threshold == 0) {
		g_debug ("not when policy is set to never");
		return;
	}

	/* get this each time, as it may have changed behind out back */
	threshold = g_settings_get_int (notifier->settings,
	                                GPK_SETTINGS_FREQUENCY_REFRESH_CACHE);
	if (threshold == 0) {
		g_debug ("not when policy is set to never");
		return;
	}

	/* get the time since the last scheduler */
	pk_control_get_time_since_action_async (notifier->control,
	                                        PK_ROLE_ENUM_REFRESH_CACHE,
	                                        NULL,
	                                        (GAsyncReadyCallback) gpk_updates_notifier_pk_get_time_since_refresh_cache_cb,
	                                        notifier);
}


/**
 *  Logic for update scheduler.
 */

static void
gpk_updates_notifier_check_updates (GpkUpdatesNotifier *notifier)
{
	/* never check for updates when offline */
	if (!gpk_updates_notifier_is_online(notifier))
		return;

	/* check if need refresh cache. */
	gpk_updates_notifier_pk_check_refresh_cache (notifier);
}

static gboolean
gpk_updates_notifier_check_hourly_cb (gpointer data)
{
	GpkUpdatesNotifier *notifier = GPK_UPDATES_NOTIFIER (data);

	g_debug ("Hourly updates check");
	gpk_updates_notifier_check_updates (notifier);

	return G_SOURCE_CONTINUE;
}

static void
gpk_updates_notifier_stop_updates_check (GpkUpdatesNotifier *notifier)
{
	if (notifier->check_hourly_id == 0)
		return;

	g_source_remove (notifier->check_hourly_id);
	notifier->check_hourly_id = 0;
}

static void
gpk_updates_notifier_restart_updates_check (GpkUpdatesNotifier *notifier)
{
	gpk_updates_notifier_stop_updates_check (notifier);
	gpk_updates_notifier_check_updates (notifier);

	notifier->check_hourly_id
		= g_timeout_add_seconds (SECONDS_IN_AN_HOUR,
		                         gpk_updates_notifier_check_hourly_cb,
		                         notifier);
}

static gboolean
gpk_updates_notifier_check_updates_on_startup_cb (gpointer user_data)
{
	GpkUpdatesNotifier *notifier = GPK_UPDATES_NOTIFIER (user_data);

	g_return_val_if_fail (GPK_IS_UPDATES_NOTIFIER (notifier), G_SOURCE_REMOVE);

	g_debug ("First hourly updates check");
	gpk_updates_notifier_restart_updates_check (notifier);

	notifier->check_startup_id = 0;

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
	GpkUpdatesNotifier *notifier = GPK_UPDATES_NOTIFIER(user_data);

	g_debug ("Xings package updates appeared on dbus. Hiding applet.");

	gpk_updates_applet_hide (notifier->applet);
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
gpk_updates_notifier_applet_activated_cb (GpkUpdatesApplet   *applet,
                                          GpkUpdatesNotifier *notifier)
{
	gpk_updates_notififier_launch_update_viewer (notifier);
}

static void
gpk_updates_notifier_notification_show_update_viewer_cb (GpkUpdatesNotification *notification,
                                                         GpkUpdatesNotifier     *notifier)
{
	gpk_updates_notififier_launch_update_viewer (notifier);
}

static void
gpk_updates_notifier_notification_ignore_updates_cb (GpkUpdatesNotification *notification,
                                                     GpkUpdatesNotifier     *notifier)
{
	g_debug ("User just ignore updates from notification...");
}


/**
 *  GpkUpdatesNotifier:
 */

static void
gpk_updates_notifier_dispose (GObject *object)
{
	GpkUpdatesNotifier *notifier;

	notifier = GPK_UPDATES_NOTIFIER (object);

	g_debug ("Stopping updates notifier");

	if (notifier->cancellable) {
		g_cancellable_cancel (notifier->cancellable);
		g_clear_object (&notifier->cancellable);
	}

	gpk_updates_notifier_stop_updates_check (notifier);

	if (notifier->check_startup_id != 0) {
		g_source_remove (notifier->check_startup_id);
		notifier->check_startup_id = 0;
	}

	if (notifier->dbus_watch_id > 0) {
		g_bus_unwatch_name (notifier->dbus_watch_id);
		notifier->dbus_watch_id = 0;
	}

	if (notifier->update_packages != NULL) {
		g_ptr_array_unref (notifier->update_packages);
		notifier->update_packages = NULL;
	}

	g_clear_object (&notifier->applet);
	g_clear_object (&notifier->notification);

	g_clear_object (&notifier->settings);
	g_clear_object (&notifier->control);
	g_clear_object (&notifier->task);

	G_OBJECT_CLASS (gpk_updates_notifier_parent_class)->dispose (object);
}

static void
gpk_updates_notifier_class_init (GpkUpdatesNotifierClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gpk_updates_notifier_dispose;
}

static void
gpk_updates_notifier_init (GpkUpdatesNotifier *notifier)
{
	g_debug ("Starting updates notifier");

	/* the notification applet */
	notifier->applet = gpk_updates_applet_new ();

	/* the notification manager */
	notifier->notification = gpk_updates_notification_new ();

	/* we need to know the updates frequency */
	notifier->settings = g_settings_new (GPK_SETTINGS_SCHEMA);

	/* PackageKit */
	notifier->control = pk_control_new ();

	notifier->task = pk_task_new ();
	g_object_set (notifier->task,
	              "background", TRUE,
	              "interactive", FALSE,
	              "only-download", TRUE,
	              NULL);

	notifier->cancellable = g_cancellable_new ();

	/* we have to consider the network connection before looking for updates */
	notifier->network_monitor = g_network_monitor_get_default ();

	/* do a first check 60 seconds after login, and then every hour */
	notifier->check_startup_id =
		g_timeout_add_seconds (60,
		                       gpk_updates_notifier_check_updates_on_startup_cb,
		                       notifier);
	g_source_set_name_by_id (notifier->check_startup_id,
	                         "[GpkUpdatesNotifier] periodic check");

	/* Check if starts the update viewer to hide the icon. */
	notifier->dbus_watch_id = g_bus_watch_name (G_BUS_TYPE_SESSION,
	                                            "org.xings.PackageUpdates",
	                                            G_BUS_NAME_WATCHER_FLAGS_NONE,
	                                            gpk_updates_viewer_appeared_cb,
	                                            gpk_updates_viewer_vanished_cb,
	                                            notifier,
	                                            NULL);

	/* show update viewer on user actions */

	g_signal_connect (notifier->applet, "activate",
			  G_CALLBACK (gpk_updates_notifier_applet_activated_cb), notifier);

	g_signal_connect (notifier->notification, "show-update-viewer",
			  G_CALLBACK (gpk_updates_notifier_notification_show_update_viewer_cb), notifier);
	g_signal_connect (notifier->notification, "ignore-updates",
			  G_CALLBACK (gpk_updates_notifier_notification_ignore_updates_cb), notifier);

	/* success */
	g_debug ("Started updates notifier");
}

GpkUpdatesNotifier *
gpk_updates_notifier_new (void)
{
	GpkUpdatesNotifier *notifier;
	notifier = g_object_new (GPK_TYPE_UPDATES_NOTIFIER, NULL);
	return GPK_UPDATES_NOTIFIER (notifier);
}

