/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2023 Matias De lellis <mati86dl@gmail.com>
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

#include <glib/gi18n.h>
#include <libnotify/notify.h>

#include <common/gpk-common.h>
#include <common/gpk-error.h>
#include <common/gpk-debug.h>

#include "gpk-flatpak-installer.h"


int
main (int argc, char *argv[])
{
	GpkFlatpakInstaller *installer = NULL;
	NotifyNotification *notification = NULL;
	GOptionContext *context;
	GError *error = NULL;
	gchar **files = NULL;
	gchar *human_size = NULL;
	gboolean ret;

	const GOptionEntry options[] = {
		{ G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &files, NULL, "[Flatpak reference file to install...]" },
		{ NULL}
	};

	setlocale (LC_ALL, "");

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	notify_init (_("Third party software installer"));

	gtk_init (&argc, &argv);

	/* TRANSLATORS: program name: application to install a package to provide a file */
	g_set_application_name (_("Third party software installer"));
	context = g_option_context_new ("flatpakref");
	g_option_context_set_summary (context, _("Install third party software from flatpakref"));
	g_option_context_add_main_entries (context, options, NULL);
	g_option_context_add_group (context, gpk_debug_get_option_group ());
	g_option_context_add_group (context, gtk_get_option_group (TRUE));
	g_option_context_parse (context, &argc, &argv, NULL);
	g_option_context_free (context);

	/* TRANSLATORS: title to pass to to the user if there are not enough privs */
	ret = gpk_check_privileged_user (_("Third party software installer"), TRUE);
	if (!ret)
		goto out;

	if (files == NULL) {
		gpk_error_dialog (_("Failed to install a third party software"),
		                  /* TRANSLATORS: nothing selected */
		                  _("You need to specify a flatpakref file to install"), NULL);
		goto out;
	}

	installer = gpk_flatpak_installer_new ();

	if (!gpk_flatpak_installer_preprare_flatpakref (installer, files[0], &error)) {
		g_debug("Failed to preprare flatpakref installation: %s\n", error->message);
		goto out;
	}

	// Just debug it as test
	human_size = g_format_size (gpk_flatpak_installer_get_download_size (installer));
	g_debug ("Download size: %s", human_size);
	g_free (human_size);

	if (!gpk_flatpak_installer_perform (installer, &error)) {
		g_debug("Failed to perform flatpakref installation: %s\n", error->message);
		goto out;
	}

	// Success, therefore we notify the user
	notification = notify_notification_new (_("Third Party Software Intalled"),
	                                        _("The requested software has been installed"),
	                                        "system-software-install");

	if (!notify_notification_show (notification, &error)) {
		g_warning ("Failed to notify the software installed: %s", error->message);
		g_clear_error (&error);
	}

	// To test the API we also launch the application
	if (!gpk_flatpak_installer_launch_ready (installer, &error)) {
		g_debug("Failed to launch flatpakref installed app: %s\n", error->message);
		g_clear_error (&error);
	}

	g_object_unref(G_OBJECT(notification));

out:
	if (error) {
		gpk_error_dialog (_("Failed to install a third party software"),
		                  error->message, NULL);
		g_clear_error (&error);
	}

	if (installer)
		g_object_unref (installer);

	if (files)
		g_strfreev (files);

	notify_uninit ();

	return !ret;
}
