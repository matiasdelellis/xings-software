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

#include <config.h>

#include <locale.h>
#include <glib/gi18n.h>
#include <glib-unix.h>
#include <libnotify/notify.h>

#include <common/gpk-debug.h>

#include "gpk-updates-manager.h"


/**
 *  Handle SIGINT and SIGTERM.
 */
static gboolean
gpk_updates_service_sigint_cb (gpointer user_data)
{
	GMainLoop *loop = user_data;
	g_debug ("Handling SIGINT");
	g_main_loop_quit (loop);
	return FALSE;
}

static gboolean
gpk_updates_service_sigterm_cb (gpointer user_data)
{
	GMainLoop *loop = user_data;
	g_debug ("Handling SIGTERM");
	g_main_loop_quit (loop);
	return FALSE;
}


/**
 * main:
 **/
int
main (int argc, char *argv[])
{
	GMainLoop *loop = NULL;
	GpkUpdatesManager *manager = NULL;
	GOptionContext *context;

	setlocale (LC_ALL, "");

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	notify_init (_("Software Update Service"));

	/* TRANSLATORS: program name, a session wide daemon to watch for updates */
	g_set_application_name (_("Software Update Service"));
	context = g_option_context_new (NULL);
	g_option_context_set_summary (context, _("Software Update Service with PackageKit"));
	g_option_context_add_group (context, gpk_debug_get_option_group ());
	g_option_context_parse (context, &argc, &argv, NULL);
	g_option_context_free (context);

	/* GpkUpdatesManager does all the magic. */

	manager = gpk_updates_manager_new ();

	/* Main loop */
	loop = g_main_loop_new (NULL, FALSE);
	g_unix_signal_add_full (G_PRIORITY_DEFAULT,
	                        SIGINT,
	                        gpk_updates_service_sigint_cb,
	                        loop,
	                        NULL);
	g_unix_signal_add_full (G_PRIORITY_DEFAULT,
	                        SIGTERM,
	                        gpk_updates_service_sigterm_cb,
	                        loop,
	                        NULL);

	/* while (1); */
	g_main_loop_run (loop);

	/* clean */
	g_main_loop_unref (loop);
	g_object_unref (manager);
	notify_uninit ();

	return 0;
}

