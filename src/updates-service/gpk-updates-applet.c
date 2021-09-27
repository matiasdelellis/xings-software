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

#include <common/gpk-common.h>

#include "gpk-updates-applet.h"

#ifdef HAVE_STATUSNOTIFIER
#include <statusnotifier.h>
#endif

struct _GpkUpdatesApplet
{
	GObject			_parent;

#ifdef HAVE_STATUSNOTIFIER
	StatusNotifierItem	*status_notifier;
#endif
};

enum {
	ACTIVATE,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GpkUpdatesApplet, gpk_updates_applet, G_TYPE_OBJECT)


static void
gpk_updates_applet_show_critical_updates (GpkUpdatesApplet *applet,
                                          gboolean          need_restart,
                                          gint              updates_count)
{
	const gchar *message;
	const gchar *title;

	/* TRANSLATORS: title in the libnotify popup */
	title = ngettext ("Update", "Updates", updates_count);

	/* TRANSLATORS: message when there are security updates */
	message = ngettext ("An important software update is available",
	                    "Important software updates are available", updates_count);

#ifdef HAVE_STATUSNOTIFIER
	g_object_set (applet->status_notifier, "title", title, NULL);
	g_object_set (applet->status_notifier, "tooltip-title", message, NULL);
	g_object_set (applet->status_notifier, "main-icon-name", GPK_ICON_UPDATES_URGENT, NULL);

	g_object_set (applet->status_notifier, "status", STATUS_NOTIFIER_STATUS_ACTIVE, NULL);
#endif

	g_debug ("applet title=%s, message=%s", title, message);
}

static void
gpk_updates_applet_show_normal_updates (GpkUpdatesApplet *applet,
                                        gboolean          need_restart,
                                        gint              updates_count)
{
	const gchar *message;
	const gchar *title;

	/* TRANSLATORS: title in the libnotify popup */
	title = ngettext ("Update", "Updates", updates_count);

	/* TRANSLATORS: message when there are non-security updates */
	message = ngettext ("A software update is available.",
	                    "Software updates are available.", updates_count);

#ifdef HAVE_STATUSNOTIFIER
	g_object_set (applet->status_notifier, "title", title, NULL);
	g_object_set (applet->status_notifier, "tooltip-title", message, NULL);
	g_object_set (applet->status_notifier, "main-icon-name", GPK_ICON_UPDATES_NORMAL, NULL);

	g_object_set (applet->status_notifier, "status", STATUS_NOTIFIER_STATUS_ACTIVE, NULL);
#endif

	g_debug ("applet title=%s, message=%s", title, message);
}

void
gpk_updates_applet_show_failed (GpkUpdatesApplet *applet)
{
	const gchar *message;
	const gchar *title;

	/* TRANSLATORS: the updates mechanism */
	title = _("Updates");

	/* TRANSLATORS: we failed to get the updates multiple times,
	 * and now we need to inform the user that something might be wrong */
	message = _("Unable to access software updates");

#ifdef HAVE_STATUSNOTIFIER
	g_object_set (applet->status_notifier, "title", title, NULL);
	g_object_set (applet->status_notifier, "tooltip-title", message, NULL);
	g_object_set (applet->status_notifier, "main-icon-name", GPK_ICON_UPDATES_URGENT, NULL);

	g_object_set (applet->status_notifier, "status", STATUS_NOTIFIER_STATUS_ACTIVE, NULL);
#endif

	g_debug ("applet title=%s, message=%s", title, message);
}

void
gpk_updates_applet_hide (GpkUpdatesApplet *applet)
{
#ifdef HAVE_STATUSNOTIFIER
	g_object_set (applet->status_notifier, "status", STATUS_NOTIFIER_STATUS_PASSIVE, NULL);
#endif
}

void
gpk_updates_applet_should_notify_updates (GpkUpdatesApplet *applet,
                                          gboolean          need_restart,
                                          guint             updates_count,
                                          guint             important_count)
{
	if (important_count) {
		gpk_updates_applet_show_critical_updates (applet, need_restart, important_count);
	} else {
		gpk_updates_applet_show_normal_updates (applet, need_restart, updates_count);
	}
}


/**
 *  GpkUpdatesApplet:
 */

#ifdef HAVE_STATUSNOTIFIER
static void
gpk_updates_applet_icon_activate_cb (GpkUpdatesApplet *applet)
{
	g_debug ("applet activated");
	g_signal_emit (applet, signals [ACTIVATE], 0);
}
#endif

static void
gpk_updates_applet_dispose (GObject *object)
{
#ifdef HAVE_STATUSNOTIFIER
	GpkUpdatesApplet *applet = GPK_UPDATES_APPLET (object);
#endif
	g_debug ("Stopping updates applet");

#ifdef HAVE_STATUSNOTIFIER
	g_clear_object (&applet->status_notifier);
#endif

	g_debug ("Stopped updates applet");

	G_OBJECT_CLASS (gpk_updates_applet_parent_class)->dispose (object);
}

static void
gpk_updates_applet_class_init (GpkUpdatesAppletClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gpk_updates_applet_dispose;

	signals [ACTIVATE] =
		g_signal_new ("activate",
		              G_TYPE_FROM_CLASS (object_class), G_SIGNAL_RUN_LAST,
		              0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0);
}

static void
gpk_updates_applet_init (GpkUpdatesApplet *applet)
{
	g_debug ("Starting updates applet");
#ifdef HAVE_STATUSNOTIFIER
	applet->status_notifier = g_object_new (STATUS_NOTIFIER_TYPE_ITEM,
	                                        "id",               "xings-updates-notifier",
	                                        "category",         STATUS_NOTIFIER_CATEGORY_SYSTEM_SERVICES,
	                                        "status",           STATUS_NOTIFIER_STATUS_PASSIVE,
	                                        "title",            "xings-updates-notifier",
	                                        "item-is-menu",     FALSE,
	                                        NULL);

	g_signal_connect_swapped (applet->status_notifier, "activate",
	                          G_CALLBACK(gpk_updates_applet_icon_activate_cb),
	                          applet);

	status_notifier_item_register (applet->status_notifier);
#endif
}

GpkUpdatesApplet *
gpk_updates_applet_new (void)
{
	GpkUpdatesApplet *applet;
	applet = g_object_new (GPK_TYPE_UPDATES_APPLET, NULL);
	return GPK_UPDATES_APPLET (applet);
}

