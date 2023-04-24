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

// TODO: Remove. Added just for flatpak error handling.
#include <flatpak.h>

#include <common/gpk-common.h>
#include <common/gpk-error.h>
#include <common/gpk-debug.h>

#include "gpk-flatpak-installer.h"


typedef struct {
	GtkApplication      *application;
	GtkBuilder          *builder;

	GpkFlatpakInstaller *installer;
	GCancellable        *cancellable;
} GpkThirdPartyAppInstallerAppPrivate;


static void
gpk_third_party_installer_done (GObject      *source_object,
                                GAsyncResult *res,
                                gpointer      user_data);


/**
 * Some helpers.
 */
static void
gpk_third_party_installer_success (GpkThirdPartyAppInstallerAppPrivate *priv, gboolean really_happened)
{
	GtkWidget *widget;
	gchar *name = NULL, *title = NULL;

	name = gpk_flatpak_installer_get_name (priv->installer);
	if (really_happened) {
		if (name)
			title = g_strdup_printf (_("“%s” has been installed"), name);
		else
			title = g_strdup (_("The app has been installed"));
	} else {
		if (name)
			title = g_strdup_printf (_("“%s” is already installed"), name);
		else
			title = g_strdup (_("This app is already installed"));
	}

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "image_badge"));
	gtk_image_set_from_icon_name (GTK_IMAGE(widget), "process-completed", GTK_ICON_SIZE_DIALOG);

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "label_title"));
	gtk_label_set_text (GTK_LABEL(widget), title);

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "label_message"));
	gtk_label_set_text (GTK_LABEL(widget), _("Open it any time from the Applications Menu"));

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "box_info"));
	gtk_widget_set_visible (widget, FALSE);

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "check_understand"));
	gtk_widget_set_visible (widget, FALSE);

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "progressbar_percent"));
	gtk_widget_set_visible (widget, FALSE);

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "button_install"));
	gtk_widget_set_visible (widget, FALSE);
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "button_cancel"));
	gtk_widget_set_visible (widget, FALSE);
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "button_open"));
	gtk_widget_set_visible (widget, TRUE);

	g_free (name);
	g_free (title);
}


/**
 * Some user interactions.
 */
static void
gpk_third_party_installer_understand_check_cb (GtkWidget *widget, GpkThirdPartyAppInstallerAppPrivate *priv)
{
	GtkWidget *button;
	button = GTK_WIDGET (gtk_builder_get_object (priv->builder, "button_install"));
	gtk_widget_set_sensitive (GTK_WIDGET (button), gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(widget)));
}

static void
gpk_third_party_installer_progress (guint    progress,
                                    gpointer user_data)
{
	GtkWidget *widget = NULL;
	GpkThirdPartyAppInstallerAppPrivate *priv = NULL;

	priv = (GpkThirdPartyAppInstallerAppPrivate *) user_data;

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "progressbar_percent"));
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(widget), (gdouble) progress/100);
}

static void
gpk_third_party_installer_install_button_cb (GtkWidget *widget, GpkThirdPartyAppInstallerAppPrivate *priv)
{
	GError *error = NULL;

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "check_understand"));
	gtk_widget_set_sensitive (widget, FALSE);

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "button_install"));
	gtk_widget_set_sensitive (widget, FALSE);

	gpk_flatpak_installer_perform_async (priv->installer,
	                                     gpk_third_party_installer_done,
	                                     gpk_third_party_installer_progress,
	                                     priv,
	                                     priv->cancellable,
	                                     &error);

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "progressbar_percent"));
	gtk_widget_set_visible (widget, TRUE);

	if (error) {
		widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "dialog_third_party_installer"));
		gpk_error_dialog_modal (GTK_WINDOW(widget),
		                        _("Failed to install a third party software"),
		                        error->message, NULL);
		g_clear_error (&error);
	}
}

static void
gpk_third_party_installer_cancel_button_cb (GtkWidget *widget, GpkThirdPartyAppInstallerAppPrivate *priv)
{
	g_cancellable_cancel (priv->cancellable);

	g_application_quit (G_APPLICATION(priv->application));
}

static void
gpk_third_party_installer_open_button_cb (GtkWidget *widget, GpkThirdPartyAppInstallerAppPrivate *priv)
{
	GError *error = NULL;

	if (!gpk_flatpak_installer_launch_ready (priv->installer, &error)) {
		g_debug("Failed to launch flatpakref installed app: %s\n", error->message);
		g_clear_error (&error);
	}

	g_application_quit (G_APPLICATION(priv->application));
}


/**
 * Flatpak Installer code
 */
static void
gpk_third_party_installer_done (GObject      *source_object,
                                GAsyncResult *res,
                                gpointer      user_data)
{
	GpkThirdPartyAppInstallerAppPrivate *priv = NULL;
	GpkFlatpakInstaller *installer = NULL;
	GError *error = NULL;

	installer = GPK_FLATPAK_INSTALLER (source_object);
	priv = (GpkThirdPartyAppInstallerAppPrivate *) user_data;

	if (!gpk_flatpak_installer_perform_finish (installer, res, &error)) {
		gpk_error_dialog (_("Failed to install a third party software"),
		                  error->message, NULL);
		g_clear_error (&error);
		return;
	}

	// Success. We inform and suggest opening the application..
	gpk_third_party_installer_success (priv, TRUE);
}


static void
gpk_third_party_installer_prepared (GObject      *source_object,
                                    GAsyncResult *res,
                                    gpointer      user_data)
{
	GtkWidget *widget = NULL;
	GpkThirdPartyAppInstallerAppPrivate *priv = NULL;
	GpkFlatpakInstaller *installer = NULL;
	gchar *human_size = NULL, *size_text = NULL;
	GError *error = NULL;

	installer = GPK_FLATPAK_INSTALLER (source_object);
	priv = (GpkThirdPartyAppInstallerAppPrivate *) user_data;

	if (!gpk_flatpak_installer_prepare_finish (installer, res, &error)) {
		if (g_error_matches (error, FLATPAK_ERROR, FLATPAK_ERROR_ALREADY_INSTALLED)) {
			g_debug("Nothing to do. This app is already installed");
			gpk_third_party_installer_success (priv, FALSE);
			g_clear_error (&error);
		} else {
			g_debug("Failed to prepare flatpakref installation: %s\n", error->message);
		}
		return;
	}

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "label_download_size"));
	human_size = g_format_size (gpk_flatpak_installer_get_download_size (priv->installer));
	size_text = g_strdup_printf (_("Download size may be up to %s"), human_size);
	gtk_label_set_text (GTK_LABEL(widget), size_text);
	g_free (human_size);
	g_free (size_text);

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "check_understand"));
	gtk_widget_set_sensitive (widget, TRUE);
}

/**
 * Gtk/G/Application
 */
static void
gpk_third_party_installer_startup_cb (GtkApplication *application, GpkThirdPartyAppInstallerAppPrivate *priv)
{
	GtkWidget *main_window;
	GtkWidget *widget;
	GError *error = NULL;
	guint retval;

	gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (),
	                                   PKGDATADIR G_DIR_SEPARATOR_S "icons");

	retval = gtk_builder_add_from_file (priv->builder, PKGDATADIR "/gpk-third-party-installer.ui", &error);
	if (retval == 0) {
		g_warning ("failed to load ui: %s", error->message);
		goto out;
	}

	main_window = GTK_WIDGET (gtk_builder_get_object (priv->builder, "dialog_third_party_installer"));
	gtk_window_set_position (GTK_WINDOW (main_window), GTK_WIN_POS_CENTER);
	gtk_window_set_icon_name (GTK_WINDOW(main_window), GPK_ICON_SOFTWARE_INSTALLER);

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "check_understand"));
	g_signal_connect (widget, "clicked",
	                  G_CALLBACK (gpk_third_party_installer_understand_check_cb), priv);

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "button_install"));
	g_signal_connect (widget, "clicked",
	                  G_CALLBACK (gpk_third_party_installer_install_button_cb), priv);

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "button_cancel"));
	g_signal_connect (widget, "clicked",
	                  G_CALLBACK (gpk_third_party_installer_cancel_button_cb), priv);

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "button_open"));
	g_signal_connect (widget, "clicked",
	                  G_CALLBACK (gpk_third_party_installer_open_button_cb), priv);

	gtk_application_add_window (application, GTK_WINDOW (main_window));
	gtk_window_present (GTK_WINDOW (main_window));

out:
	if (error) {
		gpk_error_dialog (_("Failed to install a third party software"),
		                  error->message, NULL);
		g_clear_error (&error);
	}

}

static int
gpk_third_party_installer_commandline_cb (GApplication *application,
                                          GApplicationCommandLine *cmdline,
                                          GpkThirdPartyAppInstallerAppPrivate *priv)
{
	gboolean ret;
	gchar **argv;
	gint argc;
	GOptionContext *context;
	gchar **files = NULL;
	GError *error = NULL;

	const GOptionEntry options[] = {
		{ G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &files, NULL, "[Flatpak reference file to install...]" },
		{ NULL}
	};

	/* get arguments */
	argv = g_application_command_line_get_arguments (cmdline, &argc);

	/* TRANSLATORS: program name: application to install a package to provide a file */
	g_set_application_name (_("Third Party Software Installer"));
	context = g_option_context_new ("flatpakref");
	g_option_context_set_summary (context, _("Install third party software from flatpakref"));
	g_option_context_add_main_entries (context, options, NULL);
	g_option_context_add_group (context, gpk_debug_get_option_group ());
	g_option_context_add_group (context, gtk_get_option_group (TRUE));
	ret = g_option_context_parse (context, &argc, &argv, NULL);
	if (!ret)
		goto out;

	gpk_flatpak_installer_prepare_async (priv->installer, files[0],
	                                     gpk_third_party_installer_prepared,
	                                     priv,
	                                     priv->cancellable,
	                                     &error);

	if (error != NULL) {
		g_debug("Failed to prepare flatpakref installation: %s\n", error->message);
		g_clear_error(&error);
	}

out:
	g_strfreev (argv);
	g_option_context_free (context);

	return ret;
}

int
main (int argc, char *argv[])
{
	GpkThirdPartyAppInstallerAppPrivate *priv = NULL;
	gint status = 0;

	setlocale (LC_ALL, "");

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	gtk_init (&argc, &argv);

	priv = g_new0 (GpkThirdPartyAppInstallerAppPrivate, 1);

	priv->builder = gtk_builder_new ();

	priv->installer = gpk_flatpak_installer_new ();
	priv->cancellable = g_cancellable_new ();

	priv->application = gtk_application_new ("org.xings.ThirdParty",
	                                         G_APPLICATION_HANDLES_COMMAND_LINE);

	g_signal_connect (priv->application, "startup",
	                  G_CALLBACK (gpk_third_party_installer_startup_cb), priv);
	g_signal_connect (priv->application, "command-line",
	                  G_CALLBACK (gpk_third_party_installer_commandline_cb), priv);

	/* run */
	status = g_application_run (G_APPLICATION (priv->application), argc, argv);

	g_object_unref (priv->builder);
	g_object_unref (priv->installer);
	g_object_unref (priv->cancellable);
	g_free (priv);

	return status;
}
