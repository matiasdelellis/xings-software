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
#include <libnotify/notify.h>

#include <common/gpk-common.h>
#include "gpk-updates-notification.h"

struct _GpkUpdatesNotification
{
	GObject			_parent;

	NotifyNotification	*notification_updates;

	GSettings		*settings;
};

enum {
	SHOW_UPDATE_VIEWER,
	REBOOT_SYSTEM,
	IGNORE_UPDATES,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GpkUpdatesNotification, gpk_updates_notification, G_TYPE_OBJECT)


static gboolean
gpk_updates_notification_must_show_non_critical (GpkUpdatesNotification *notification)
{
	guint64 time_now, time_last_notify;
	guint threshold;

	/* find out if enough time has passed since the last notification */
	time_now = g_get_real_time () / G_USEC_PER_SEC;
	threshold = g_settings_get_int (notification->settings,
	                                GPK_SETTINGS_FREQUENCY_UPDATES_NOTIFICATION);

	time_last_notify = g_settings_get_uint64 (notification->settings,
	                                          GPK_SETTINGS_LAST_UPDATES_NOTIFICATION);

	if ((guint64) threshold > time_now - time_last_notify) {
		g_debug ("not must show non-critical updates as already shown %i hours ago",
		        (guint) (time_now - time_last_notify) / (60 * 60));
		return FALSE;
	}

	return TRUE;
}

static void
gpk_updates_notification_response_action (NotifyNotification *update_notification,
                                          gchar              *action,
                                          gpointer            user_data)
{
	gboolean ret;
	GError *error = NULL;

	GpkUpdatesNotification *notification = GPK_UPDATES_NOTIFICATION (user_data);

	notify_notification_close (update_notification, NULL);

	if (g_strcmp0 (action, "ignore") == 0) {
		g_debug ("notification ignore updates");
		g_signal_emit (notification, signals [IGNORE_UPDATES], 0);
		goto out;
	}

	if (g_strcmp0 (action, "show-update-viewer") == 0) {
		g_debug ("notification show updates");
		g_signal_emit (notification, signals [SHOW_UPDATE_VIEWER], 0);
		goto out;
	}

	if (g_strcmp0 (action, "reboot-system") == 0) {
		g_debug ("notification reboot system");
		g_signal_emit (notification, signals [REBOOT_SYSTEM], 0);
		goto out;
	}

	g_warning ("unknown notification action id: %s", action);

out:
	return;
}

static void
gpk_updates_notification_closed (NotifyNotification *notification, gpointer data)
{
	g_object_unref (notification);
}

static void
gpk_updates_notification_show_critical_updates (GpkUpdatesNotification *notification,
                                                gboolean                need_restart,
                                                gint                    updates_count)
{
	NotifyNotification *notification_updates;
	const gchar *message;
	const gchar *title;
	gboolean ret;
	GError *error = NULL;

	/* TRANSLATORS: title in the libnotify popup */
	title = ngettext ("Update", "Updates", updates_count);

	/* TRANSLATORS: message when there are security updates */
	message = ngettext ("An important software update is available",
	                    "Important software updates are available", updates_count);

	/* close any existing notification */
	if (notification->notification_updates != NULL) {
		notify_notification_close (notification->notification_updates, NULL);
		notification->notification_updates = NULL;
	}

	/* do the bubble */
	g_debug ("title=%s, message=%s", title, message);
	notification_updates = notify_notification_new (title,
	                                                message,
	                                                GPK_ICON_UPDATES_URGENT);

	notify_notification_set_app_name (notification_updates,
	                                  _("Software Updates"));
	notify_notification_set_timeout (notification_updates,
	                                 15000);
	notify_notification_set_urgency (notification_updates,
	                                 NOTIFY_URGENCY_CRITICAL);
	notify_notification_set_hint_string (notification_updates,
	                                     "desktop-entry",
	                                     "xings-package-updates");

	notify_notification_add_action (notification_updates,
	                                "ignore",
	                                /* TRANSLATORS: don't install updates now */
	                                _("Not Now"),
	                                gpk_updates_notification_response_action,
	                                notification, NULL);

	notify_notification_add_action (notification_updates,
	                                "show-update-viewer",
	                                /* TRANSLATORS: view available updates */
	                                _("View"),
	                                gpk_updates_notification_response_action,
	                                notification, NULL);

	if (need_restart) {
		notify_notification_add_action (notification_updates,
		                                "reboot-system",
		                                /* TRANSLATORS: install available updates */
		                                _("Restart & Install"),
		                                gpk_updates_notification_response_action,
		                                notification, NULL);
	}

	g_signal_connect (notification_updates, "closed",
	                  G_CALLBACK (gpk_updates_notification_closed), NULL);

	ret = notify_notification_show (notification_updates, &error);
	if (!ret) {
		g_warning ("error: %s", error->message);
		g_error_free (error);
	}

	/* track so we can prevent doubled notifications */
	notification->notification_updates = notification_updates;
	g_object_add_weak_pointer (G_OBJECT (notification->notification_updates),
	                           (void **) &notification->notification_updates);
}

static void
gpk_updates_notification_maybe_show_normal_updates (GpkUpdatesNotification *notification,
                                                    gboolean                need_restart,
                                                    gint                    updates_count)
{
	NotifyNotification *notification_updates;
	const gchar *message;
	const gchar *title;
	gboolean ret;
	GError *error = NULL;

	if (!gpk_updates_notification_must_show_non_critical(notification))
		return;

	/* TRANSLATORS: title in the libnotify popup */
	title = ngettext ("Update", "Updates", updates_count);

	/* TRANSLATORS: message when there are non-security updates */
	message = ngettext ("A software update is available.",
	                    "Software updates are available.", updates_count);

	/* close any existing notification */
	if (notification->notification_updates != NULL) {
		notify_notification_close (notification->notification_updates, NULL);
		notification->notification_updates = NULL;
	}

	/* do the bubble */
	g_debug ("title=%s, message=%s", title, message);
	notification_updates = notify_notification_new (title,
	                                                message,
	                                                GPK_ICON_UPDATES_NORMAL);

	notify_notification_set_app_name (notification_updates,
	                                  _("Software Updates"));
	notify_notification_set_timeout (notification_updates,
	                                 15000);
	notify_notification_set_urgency (notification_updates,
	                                 NOTIFY_URGENCY_NORMAL);
	notify_notification_set_hint_string (notification_updates,
	                                     "desktop-entry",
	                                     "xings-package-updates");

	notify_notification_add_action (notification_updates,
	                                "ignore",
	                                /* TRANSLATORS: don't install updates now */
	                                _("Not Now"),
	                                gpk_updates_notification_response_action,
	                                notification, NULL);

	notify_notification_add_action (notification_updates,
	                                "show-update-viewer",
	                                /* TRANSLATORS: view available updates */
	                                _("View"),
	                                gpk_updates_notification_response_action,
	                                notification, NULL);

	if (need_restart) {
		notify_notification_add_action (notification_updates,
		                                "reboot-system",
		                                /* TRANSLATORS: install available updates */
		                                _("Restart & Install"),
		                                gpk_updates_notification_response_action,
		                                notification, NULL);
	}

	g_signal_connect (notification_updates, "closed",
	                  G_CALLBACK (gpk_updates_notification_closed), NULL);

	ret = notify_notification_show (notification_updates, &error);
	if (!ret) {
		g_warning ("error: %s", error->message);
		g_error_free (error);
	}

	/* reset notification time */
	g_settings_set_uint64 (notification->settings,
	                       GPK_SETTINGS_LAST_UPDATES_NOTIFICATION,
	                       g_get_real_time () / G_USEC_PER_SEC);

	/* track so we can prevent doubled notifications */
	notification->notification_updates = notification_updates;
	g_object_add_weak_pointer (G_OBJECT (notification->notification_updates),
	                           (void **) &notification->notification_updates);
}

void
gpk_updates_notification_show_failed (GpkUpdatesNotification *notification)
{
	NotifyNotification *notification_updates;
	const gchar *button;
	const gchar *message;
	const gchar *title;
	gboolean ret;
	GError *error = NULL;

	/* TRANSLATORS: the updates mechanism */
	title = _("Updates");

	/* TRANSLATORS: we failed to get the updates multiple times,
	 * and now we need to inform the user that something might be wrong */
	message = _("Unable to access software updates");

	/* TRANSLATORS: try again, this time launching the update viewer */
	button = _("Try again");

	notification_updates = notify_notification_new (title,
	                                                message,
	                                                GPK_ICON_UPDATES_NORMAL);

	notify_notification_set_app_name (notification_updates,
	                                  _("Software Updates"));
	notify_notification_set_timeout (notification_updates,
	                                 120*1000);
	notify_notification_set_urgency (notification_updates,
	                                 NOTIFY_URGENCY_NORMAL);
	notify_notification_set_hint_string (notification_updates,
	                                     "desktop-entry",
	                                     "xings-package-updates");

	notify_notification_add_action (notification_updates,
	                                "show-update-viewer",
	                                button,
	                                gpk_updates_notification_response_action,
	                                notification, NULL);

	g_signal_connect (notification_updates, "closed",
	                  G_CALLBACK (gpk_updates_notification_closed), NULL);

	ret = notify_notification_show (notification_updates, &error);
	if (!ret) {
		g_warning ("failed to show notification: %s", error->message);
		g_error_free (error);
	}
}

void
gpk_updates_notification_should_notify_updates (GpkUpdatesNotification *notification,
                                                gboolean                need_restart,
                                                guint                   updates_count,
                                                guint                   important_count)
{
	if (important_count) {
		gpk_updates_notification_show_critical_updates (notification,
		                                                need_restart,
		                                                important_count);
	} else {
		gpk_updates_notification_maybe_show_normal_updates (notification,
		                                                    need_restart,
		                                                    updates_count);
	}
}

/**
 *  GpkUpdatesNotification:
 */

static void
gpk_updates_notification_dispose (GObject *object)
{
	GpkUpdatesNotification *notification;

	notification = GPK_UPDATES_NOTIFICATION (object);

	g_debug ("Stopping updates notification");

	g_clear_object (&notification->settings);

	G_OBJECT_CLASS (gpk_updates_notification_parent_class)->dispose (object);
}

static void
gpk_updates_notification_class_init (GpkUpdatesNotificationClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gpk_updates_notification_dispose;

	signals [SHOW_UPDATE_VIEWER] =
		g_signal_new ("show-update-viewer",
		              G_TYPE_FROM_CLASS (object_class), G_SIGNAL_RUN_LAST,
		              0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0);

	signals [IGNORE_UPDATES] =
		g_signal_new ("ignore-updates",
		              G_TYPE_FROM_CLASS (object_class), G_SIGNAL_RUN_LAST,
		              0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0);

	signals [REBOOT_SYSTEM] =
		g_signal_new ("reboot-system",
		              G_TYPE_FROM_CLASS (object_class), G_SIGNAL_RUN_LAST,
		              0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0);
}

static void
gpk_updates_notification_init (GpkUpdatesNotification *notification)
{
	g_debug ("Starting updates notification");

	/* we need to know the updates frequency */
	notification->settings = g_settings_new (GPK_SETTINGS_SCHEMA);

	g_debug ("Started updates notification");
}

GpkUpdatesNotification *
gpk_updates_notification_new (void)
{
	GpkUpdatesNotification *notification;
	notification = g_object_new (GPK_TYPE_UPDATES_NOTIFICATION, NULL);
	return GPK_UPDATES_NOTIFICATION (notification);
}

