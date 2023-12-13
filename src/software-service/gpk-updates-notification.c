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

#ifdef HAVE_STATUSNOTIFIER
#include <statusnotifier.h>
#endif

#include <common/gpk-common.h>
#ifdef BUILD_OFFLINE_UPDATES
#include <common/gpk-session.h>
#endif

#include "gpk-updates-shared.h"

#include "gpk-updates-notification.h"

struct _GpkUpdatesNotification
{
	GObject			_parent;

	NotifyNotification	*notification_updates;

#ifdef BUILD_OFFLINE_UPDATES
	GpkSession		*session;
#endif

#ifdef HAVE_STATUSNOTIFIER
	StatusNotifierItem	*status_notifier;
#endif

	GpkUpdatesShared	*shared;
};

enum {
	SHOW_UPDATE_VIEWER,
	REBOOT_SYSTEM,
	IGNORE_UPDATES,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GpkUpdatesNotification, gpk_updates_notification, G_TYPE_OBJECT)


#ifdef HAVE_STATUSNOTIFIER
static void
gpk_updates_notification_show_applet (GpkUpdatesNotification *notification,
                                      const gchar            *title,
                                      const gchar            *message,
                                      const gchar            *icon_name)
{
	g_object_set (notification->status_notifier,
	              "title", title, NULL);
	g_object_set (notification->status_notifier,
	              "tooltip-title", message, NULL);
	g_object_set (notification->status_notifier,
	              "main-icon-name", icon_name, NULL);

	g_object_set (notification->status_notifier,
	              "status", STATUS_NOTIFIER_STATUS_ACTIVE, NULL);

	g_debug ("show applet title=%s, message=%s", title, message);
}

static void
gpk_updates_notification_hide_applet (GpkUpdatesNotification *notification)
{
	g_object_set (notification->status_notifier,
	              "status", STATUS_NOTIFIER_STATUS_PASSIVE,
	              NULL);
	g_debug ("hidding applet");
}
#endif

static void
gpk_updates_notification_ignore_action (GpkUpdatesNotification *notification)
{
#ifdef HAVE_STATUSNOTIFIER
	gpk_updates_notification_hide_applet (notification);
#endif
	g_signal_emit (notification, signals [IGNORE_UPDATES], 0);
}

static void
gpk_updates_notification_view_action (GpkUpdatesNotification *notification)
{
#ifdef HAVE_STATUSNOTIFIER
	gpk_updates_notification_hide_applet (notification);
#endif
	g_signal_emit (notification, signals [SHOW_UPDATE_VIEWER], 0);
}

static void
gpk_updates_notification_reboot_system_action (GpkUpdatesNotification *notification)
{
#ifdef HAVE_STATUSNOTIFIER
	gpk_updates_notification_hide_applet (notification);
#endif
	g_signal_emit (notification, signals [REBOOT_SYSTEM], 0);
}


static void
gpk_updates_notification_response_action (NotifyNotification *update_notification,
                                          gchar              *action,
                                          gpointer            user_data)
{
	GpkUpdatesNotification *notification = GPK_UPDATES_NOTIFICATION (user_data);

	notify_notification_close (update_notification, NULL);

	if (g_strcmp0 (action, "ignore") == 0) {
		g_debug ("notification ignore updates");
		gpk_updates_notification_ignore_action (notification);
		goto out;
	}

	if (g_strcmp0 (action, "show-update-viewer") == 0) {
		g_debug ("notification show updates");
		gpk_updates_notification_view_action (notification);
		goto out;
	}

	if (g_strcmp0 (action, "reboot-system") == 0) {
		g_debug ("notification reboot system");
		gpk_updates_notification_reboot_system_action (notification);
		goto out;
	}

	g_warning ("unknown notification action id: %s", action);

out:
	return;
}

static void
gpk_updates_notification_closed (NotifyNotification     *notification_updates,
                                 GpkUpdatesNotification *notification)
{
	notification->notification_updates = NULL;
	g_object_unref (notification_updates);
}

static void
gpk_updates_notification_show (GpkUpdatesNotification *notification,
                               const gchar            *title,
                               const gchar            *message,
                               const gchar            *icon_name,
                               gboolean                downloaded)
{
	NotifyNotification *notification_updates;
	gboolean ret, can_reboot = FALSE;
	GError *error = NULL;

	if (notification->notification_updates != NULL) {
		if (!notify_notification_close (notification->notification_updates, &error)) {
			g_printerr ("Failed to close notification: %s", error->message);
			g_clear_error (&error);
			return;
		}
	}

	/* do the bubble */

	notification_updates = notify_notification_new (title,
	                                                message,
	                                                icon_name);

	notify_notification_set_app_name (notification_updates,
	                                  _("Software Update"));
	notify_notification_set_timeout (notification_updates,
	                                 15000);
	notify_notification_set_urgency (notification_updates,
	                                 NOTIFY_URGENCY_CRITICAL);
	notify_notification_set_hint_string (notification_updates,
	                                     "desktop-entry",
	                                     "xings-software-update");

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

#ifdef BUILD_OFFLINE_UPDATES
	if (downloaded) {
		gpk_session_can_reboot (notification->session, &can_reboot, NULL);
		if (can_reboot) {
			notify_notification_add_action (notification_updates,
			                                "reboot-system",
			                                /* TRANSLATORS: install available updates */
			                                _("Restart & Install"),
			                                gpk_updates_notification_response_action,
			                                notification, NULL);
		}
	}
#endif

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
gpk_updates_notification_show_critical_updates (GpkUpdatesNotification *notification,
                                                gboolean                downloaded,
                                                gint                    updates_count)
{
	const gchar *message;
	const gchar *title;

	/* TRANSLATORS: title in the libnotify popup */
	title = ngettext ("Update", "Updates", updates_count);

	/* TRANSLATORS: message when there are security updates */
	message = ngettext ("An important software update is available",
	                    "Important software updates are available", updates_count);

	/* do the bubble */

	gpk_updates_notification_show (notification,
	                               title,
	                               message,
	                               GPK_ICON_UPDATES_URGENT,
	                               downloaded);

#ifdef HAVE_STATUSNOTIFIER
	gpk_updates_notification_show_applet (notification,
	                                      title,
	                                      message,
	                                      GPK_ICON_UPDATES_URGENT);
#endif

	g_debug ("notification title=%s, message=%s", title, message);
}

static void
gpk_updates_notification_maybe_show_normal_updates (GpkUpdatesNotification *notification,
                                                    gboolean                downloaded,
                                                    gint                    updates_count)
{
	const gchar *message;
	const gchar *title;

	if (!gpk_updates_shared_must_show_non_critical(notification->shared))
		return;

	/* TRANSLATORS: title in the libnotify popup */
	title = ngettext ("Update", "Updates", updates_count);

	/* TRANSLATORS: message when there are non-security updates */
	message = ngettext ("A software update is available.",
	                    "Software updates are available.", updates_count);

	/* do the bubble */

	gpk_updates_notification_show (notification,
	                               title,
	                               message,
	                               GPK_ICON_UPDATES_NORMAL,
	                               downloaded);

#ifdef HAVE_STATUSNOTIFIER
	gpk_updates_notification_show_applet (notification,
	                                      title,
	                                      message,
	                                      GPK_ICON_UPDATES_NORMAL);
#endif

	/* reset notification time */
	gpk_updates_shared_reset_show_non_critical (notification->shared);
}

void
gpk_updates_notification_show_failed (GpkUpdatesNotification *notification)
{
	const gchar *message;
	const gchar *title;

	/* TRANSLATORS: the updates mechanism */
	title = _("Updates");

	/* TRANSLATORS: we failed to get the updates multiple times,
	 * and now we need to inform the user that something might be wrong */
	message = _("Unable to access software updates");

	gpk_updates_notification_show (notification,
	                               title,
	                               message,
	                               GPK_ICON_UPDATES_URGENT,
	                               FALSE);

#ifdef HAVE_STATUSNOTIFIER
	gpk_updates_notification_show_applet (notification,
	                                      title,
	                                      message,
	                                      GPK_ICON_UPDATES_URGENT);
#endif
}

void
gpk_updates_notification_should_notify_updates (GpkUpdatesNotification *notification,
                                                gboolean                downloaded,
                                                guint                   updates_count,
                                                guint                   important_count)
{
	if (important_count) {
		gpk_updates_notification_show_critical_updates (notification,
		                                                downloaded,
		                                                important_count);
	} else {
		gpk_updates_notification_maybe_show_normal_updates (notification,
		                                                    downloaded,
		                                                    updates_count);
	}
}

#ifdef HAVE_STATUSNOTIFIER
static void
gpk_updates_notification_applet_icon_activate_cb (GpkUpdatesNotification *notification)
{
	g_debug ("applet activated");
	gpk_updates_notification_hide_applet (notification);
	g_signal_emit (notification, signals [SHOW_UPDATE_VIEWER], 0);
}
#endif


/**
 *  GpkUpdatesNotification:
 */
static void
gpk_updates_notification_dispose (GObject *object)
{
	GpkUpdatesNotification *notification;

	notification = GPK_UPDATES_NOTIFICATION (object);

	g_debug ("Stopping updates notification");

#ifdef BUILD_OFFLINE_UPDATES
	g_clear_object (&notification->session);
#endif

#ifdef HAVE_STATUSNOTIFIER
	g_clear_object (&notification->status_notifier);
#endif

	g_clear_object (&notification->shared);

	g_debug ("Stopped updates notification");

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

	notification->shared = gpk_updates_shared_get ();

#ifdef BUILD_OFFLINE_UPDATES
	notification->session = gpk_session_new ();
#endif

#ifdef HAVE_STATUSNOTIFIER
	notification->status_notifier = g_object_new (STATUS_NOTIFIER_TYPE_ITEM,
	                                              "id",           "xings-updates-notifier",
	                                              "category",     STATUS_NOTIFIER_CATEGORY_SYSTEM_SERVICES,
	                                              "status",       STATUS_NOTIFIER_STATUS_PASSIVE,
	                                              "title",        _("Software Updates"),
	                                              "item-is-menu", FALSE,
	                                              NULL);

	g_signal_connect_swapped (notification->status_notifier, "activate",
	                          G_CALLBACK(gpk_updates_notification_applet_icon_activate_cb),
	                          notification);

	status_notifier_item_register (notification->status_notifier);
#endif

	g_debug ("Started updates notification");
}

GpkUpdatesNotification *
gpk_updates_notification_new (void)
{
	GpkUpdatesNotification *notification;
	notification = g_object_new (GPK_TYPE_UPDATES_NOTIFICATION, NULL);
	return GPK_UPDATES_NOTIFICATION (notification);
}

