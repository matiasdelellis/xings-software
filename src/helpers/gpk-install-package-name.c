/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007-2008 Richard Hughes <richard@hughsie.com>
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
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libnotify/notify.h>

#include <common/gpk-common.h>
#include <common/gpk-error.h>
#include <common/gpk-debug.h>

/**
 * main:
 **/
int
main (int argc, char *argv[])
{
	NotifyNotification *notification = NULL;
	GOptionContext *context;
	GDBusProxy *proxy = NULL;
	GVariantBuilder  array_builder;
	GVariant *output;
	GError *error = NULL;
	gchar **packages = NULL;
	guint i = 0;
	gboolean ret;

	const GOptionEntry options[] = {
		{ G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_STRING_ARRAY, &packages,
		  _("Distribution packages to install"), NULL },
		{ NULL}
	};

	setlocale (LC_ALL, "");

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	notify_init (_("Software Installer"));

	gtk_init (&argc, &argv);

	/* TRANSLATORS: program name: installs a package (or packages) by name */
	g_set_application_name (_("Software Installer"));
	context = g_option_context_new ("PACKAGE1 PACKAGE2…");
	g_option_context_set_summary (context, _("Install software with the name of the distribution packages"));
	g_option_context_add_main_entries (context, options, NULL);
	g_option_context_add_group (context, gpk_debug_get_option_group ());
	g_option_context_add_group (context, gtk_get_option_group (TRUE));
	g_option_context_parse (context, &argc, &argv, NULL);
	g_option_context_free (context);

	/* TRANSLATORS: application name to pass to to the user if there are not enough privs */
	ret = gpk_check_privileged_user (_("Software Installer"), TRUE);
	if (!ret)
		goto out;

	if (packages == NULL) {
		/* TRANSLATORS: failed */
		gpk_error_dialog (_("Failed to install software by name"),
				  /* TRANSLATORS: nothing was specified */
				  _("You need to specify the name of the software to install"), NULL);
		goto out;
	}

	proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
	                                       G_DBUS_PROXY_FLAGS_NONE,
	                                       NULL,
	                                       "org.freedesktop.PackageKit",
	                                       "/org/freedesktop/PackageKit",
	                                       "org.freedesktop.PackageKit.Modify",
	                                       NULL,
	                                       &error);

	if (proxy == NULL) {
		g_warning ("Cannot connect to session service: %s", error->message);
		g_error_free (error);
		goto out;
	}

	g_variant_builder_init (&array_builder, G_VARIANT_TYPE ("as"));
	for (i = 0; packages[i] != NULL ; i++) {
		g_variant_builder_add (&array_builder,
		                       "s",
		                       packages[i]);
	}

	output = g_dbus_proxy_call_sync (proxy,
	                                 "InstallPackageNames",
	                                 g_variant_new ("(uass)",
	                                                0, /* fake xid */
	                                                &array_builder, /* data */
	                                                "hide-finished,show-warnings"), /* interactions */
	                                 G_DBUS_CALL_FLAGS_NONE,
	                                 G_MAXINT,
	                                 NULL,
	                                 &error);

	if (!output) {
		g_warning ("Cannot install packages: %s", error->message);
		g_error_free (error);
		goto out;
	}

	notification = notify_notification_new (_("Software Installed"),
	                                        _("The requested software has been installed"),
	                                        "system-software-install");

	if (!notify_notification_show (notification, &error)) {
		g_warning ("Cannot notify the software installed: %s", error->message);
		g_error_free (error);
	}

	g_object_unref(G_OBJECT(notification));
	g_variant_unref (output);

out:
	if (proxy != NULL)
		g_object_unref (proxy);
	g_strfreev (packages);

	notify_uninit ();

	return !ret;
}
