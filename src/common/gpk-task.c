/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2009 Richard Hughes <richard@hughsie.com>
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
#include <packagekit-glib2/packagekit.h>

#include "gpk-task.h"
#include "gpk-gnome.h"
#include "gpk-common.h"
#include "gpk-enum.h"
#include "gpk-dialog.h"

static void     gpk_task_finalize	(GObject     *object);

/**
 * GpkTask:
 *
 **/
struct _GpkTask
{
	PkTask			_parent_instance;
	gpointer		 user_data;
	GSettings		*settings;
	GtkWindow		*parent_window;
	GtkWindow		*current_window;
	GtkBuilder		*builder_untrusted;
	GtkBuilder		*builder_signature;
	GtkBuilder		*builder_eula;
	guint			 request;
	const gchar		*help_id;
};

G_DEFINE_TYPE (GpkTask, gpk_task, PK_TYPE_TASK)

/**
 * gpk_task_set_parent_window:
 **/
gboolean
gpk_task_set_parent_window (GpkTask *task, GtkWindow *parent_window)
{
	g_return_val_if_fail (GPK_IS_TASK (task), FALSE);
	g_return_val_if_fail (parent_window != NULL, FALSE);
	task->parent_window = parent_window;
	return TRUE;
}

/**
 * gpk_task_button_accept_cb:
 **/
static void
gpk_task_button_accept_cb (GtkWidget *widget, GpkTask *task)
{
	gtk_widget_hide (GTK_WIDGET(task->current_window));
	pk_task_user_accepted (PK_TASK(task), task->request);
	task->request = 0;
	task->current_window = NULL;
}

/**
 * gpk_task_button_decline_cb:
 **/
static void
gpk_task_button_decline_cb (GtkWidget *widget, GpkTask *task)
{
	gtk_widget_hide (GTK_WIDGET(task->current_window));
	pk_task_user_declined (PK_TASK(task), task->request);
	task->request = 0;
	task->current_window = NULL;
}

/**
 * gpk_task_dialog_response_cb:
 **/
static void
gpk_task_dialog_response_cb (GtkDialog *dialog, gint response_id, GpkTask *task)
{
	if (response_id == GTK_RESPONSE_YES) {
		gpk_task_button_accept_cb (GTK_WIDGET(dialog), task);
		return;
	}
	/* all other options */
	gpk_task_button_decline_cb (GTK_WIDGET(dialog), task);
}

/**
 * gpk_task_untrusted_question:
 **/
static void
gpk_task_untrusted_question (PkTask *task, guint request, PkResults *results)
{
	GtkWidget *widget;
	gchar *message;
	PkRoleEnum role;

	GpkTask *gpk_task = GPK_TASK(task);

	/* save the current request */
	gpk_task->request = request;

	/* title */
	widget = GTK_WIDGET(gtk_builder_get_object (gpk_task->builder_untrusted, "label_title"));
	gtk_widget_hide (widget);

	/* message */
	g_object_get (results, "role", &role, NULL);
	if (role == PK_ROLE_ENUM_UPDATE_PACKAGES) {
		message = g_strdup_printf ("%s\n%s\n\n%s\n%s",
					   /* TRANSLATORS: is not GPG signed */
					   _("The software is not signed by a trusted provider."),
					   /* TRANSLATORS: user has to trust provider -- I know, this sucks */
					   _("Do not update this package unless you are sure it is safe to do so."),
					   /* TRANSLATORS: warn the user that all bets are off */
					   _("Malicious software can damage your computer or cause other harm."),
					   /* TRANSLATORS: ask if they are absolutely sure they want to do this */
					   _("Are you <b>sure</b> you want to update this package?"));
	} else {
		message = g_strdup_printf ("%s\n%s\n\n%s\n%s",
					   /* TRANSLATORS: is not GPG signed */
					   _("The software is not signed by a trusted provider."),
					   /* TRANSLATORS: user has to trust provider -- I know, this sucks */
					   _("Do not install this package unless you are sure it is safe to do so."),
					   /* TRANSLATORS: warn the user that all bets are off */
					   _("Malicious software can damage your computer or cause other harm."),
					   /* TRANSLATORS: ask if they are absolutely sure they want to do this */
					   _("Are you <b>sure</b> you want to install this package?"));
	}
	widget = GTK_WIDGET (gtk_builder_get_object (gpk_task->builder_untrusted, "label_message"));
	gtk_label_set_markup (GTK_LABEL (widget), message);
	g_free (message);

	/* show window */
	gpk_task->current_window = GTK_WINDOW(gtk_builder_get_object (gpk_task->builder_untrusted, "dialog_error"));
	if (gpk_task->parent_window != NULL) {
		gtk_window_set_transient_for (gpk_task->current_window, gpk_task->parent_window);
		gtk_window_set_modal (gpk_task->current_window, TRUE);
		/* this is a modal popup, so don't show a window title */
		gtk_window_set_title (gpk_task->current_window, "");
	}
	gtk_window_set_title (gpk_task->current_window, _("The software is not signed by a trusted provider."));
	gtk_widget_show (GTK_WIDGET(gpk_task->current_window));
}

/**
 * gpk_task_key_question:
 **/
static void
gpk_task_key_question (PkTask *task, guint request, PkResults *results)
{
	GPtrArray *array;
	GtkWidget *widget;
	gchar *printable = NULL;
	gchar *package_id = NULL;
	gchar *repository_name = NULL;
	gchar *key_url = NULL;
	gchar *key_userid = NULL;
	gchar *key_id = NULL;
	PkRepoSignatureRequired *item;

	GpkTask *gpk_task = GPK_TASK(task);

	/* save the current request */
	gpk_task->request = request;

	/* get data */
	array = pk_results_get_repo_signature_required_array (results);
	if (array->len != 1) {
		g_warning ("array length %u, aborting", array->len);
		goto out;
	}

	/* only one item supported */
	item = g_ptr_array_index (array, 0);
	g_object_get (item,
		      "package-id", &package_id,
		      "repository-name", &repository_name,
		      "key-url", &key_url,
		      "key-userid", &key_userid,
		      "key-id", &key_id,
		      NULL);

	/* show correct text */
	widget = GTK_WIDGET (gtk_builder_get_object (gpk_task->builder_signature, "label_name"));
	gtk_label_set_label (GTK_LABEL (widget), repository_name);
	widget = GTK_WIDGET (gtk_builder_get_object (gpk_task->builder_signature, "label_url"));
	gtk_label_set_label (GTK_LABEL (widget), key_url);
	widget = GTK_WIDGET (gtk_builder_get_object (gpk_task->builder_signature, "label_user"));
	gtk_label_set_label (GTK_LABEL (widget), key_userid);
	widget = GTK_WIDGET (gtk_builder_get_object (gpk_task->builder_signature, "label_id"));
	gtk_label_set_label (GTK_LABEL (widget), key_id);

	printable = pk_package_id_to_printable (package_id);
	widget = GTK_WIDGET (gtk_builder_get_object (gpk_task->builder_signature, "label_package"));
	gtk_label_set_label (GTK_LABEL (widget), printable);

	/* show window */
	gpk_task->current_window = GTK_WINDOW(gtk_builder_get_object (gpk_task->builder_signature, "dialog_gpg"));
	if (gpk_task->parent_window != NULL) {
		gtk_window_set_transient_for (gpk_task->current_window, gpk_task->parent_window);
		gtk_window_set_modal (gpk_task->current_window, TRUE);
		/* this is a modal popup, so don't show a window title */
		gtk_window_set_title (gpk_task->current_window, "");
	}
	gpk_task->help_id = "gpg-signature";
	gtk_widget_show (GTK_WIDGET(gpk_task->current_window));
out:
	g_free (printable);
	g_free (package_id);
	g_free (repository_name);
	g_free (key_url);
	g_free (key_userid);
	g_free (key_id);
	g_ptr_array_unref (array);
}

/**
 * gpk_task_eula_question:
 **/
static void
gpk_task_eula_question (PkTask *task, guint request, PkResults *results)
{
	GPtrArray *array;
	GtkWidget *widget;
	GtkTextBuffer *buffer;
	gchar *printable = NULL;
	gchar **split = NULL;
	PkEulaRequired *item;
	gchar *package_id = NULL;
	gchar *vendor_name = NULL;
	gchar *license_agreement = NULL;

	GpkTask *gpk_task = GPK_TASK(task);

	/* save the current request */
	gpk_task->request = request;

	/* get data */
	array = pk_results_get_eula_required_array (results);
	if (array->len != 1) {
		g_warning ("array length %u, aborting", array->len);
		goto out;
	}

	/* only one item supported */
	item = g_ptr_array_index (array, 0);
	g_object_get (item,
		      "package-id", &package_id,
		      "vendor-name", &vendor_name,
		      "license-agreement", &license_agreement,
		      NULL);

	/* title */
	widget = GTK_WIDGET (gtk_builder_get_object (gpk_task->builder_eula, "label_title"));

	split = pk_package_id_split (package_id);
	printable = g_markup_printf_escaped("<b><big>License required for %s by %s</big></b>", split[0], vendor_name);
	gtk_label_set_label (GTK_LABEL (widget), printable);

	buffer = gtk_text_buffer_new (NULL);
	gtk_text_buffer_insert_at_cursor (buffer, license_agreement, strlen (license_agreement));
	widget = GTK_WIDGET (gtk_builder_get_object (gpk_task->builder_eula, "textview_details"));
	gtk_text_view_set_buffer (GTK_TEXT_VIEW (widget), buffer);

	/* set minimum size a bit bigger */
	gtk_widget_set_size_request (widget, 100, 200);

	/* show window */
	gpk_task->current_window = GTK_WINDOW(gtk_builder_get_object (gpk_task->builder_eula, "dialog_eula"));
	if (gpk_task->parent_window != NULL) {
		gtk_window_set_transient_for (gpk_task->current_window, gpk_task->parent_window);
		gtk_window_set_modal (gpk_task->current_window, TRUE);
		/* this is a modal popup, so don't show a window title */
		gtk_window_set_title (gpk_task->current_window, "");
	}
	gpk_task->help_id = "eula";
	gtk_widget_show (GTK_WIDGET(gpk_task->current_window));

	g_object_unref (buffer);
out:
	g_free (printable);
	g_free (package_id);
	g_free (vendor_name);
	g_free (license_agreement);
	g_strfreev (split);
	g_ptr_array_unref (array);
}

/**
 * gpk_task_media_change_question:
 **/
static void
gpk_task_media_change_question (PkTask *task, guint request, PkResults *results)
{
	GPtrArray *array;
	PkMediaChangeRequired *item;
	const gchar *name;
	gchar *message = NULL;
	gchar *media_id;
	PkMediaTypeEnum media_type;
	gchar *media_text;

	GpkTask *gpk_task = GPK_TASK(task);

	/* save the current request */
	gpk_task->request = request;

	/* get data */
	array = pk_results_get_media_change_required_array (results);
	if (array->len != 1) {
		g_warning ("array length %u, aborting", array->len);
		goto out;
	}

	/* only one item supported */
	item = g_ptr_array_index (array, 0);
	g_object_get (item,
		      "media-id", &media_id,
		      "media-type", &media_type,
		      "media-text", &media_text,
		      NULL);

	name = gpk_media_type_enum_to_localised_text (media_type);
	/* TRANSLATORS: dialog body, explains to the user that they need to insert a disk to continue. The first replacement is DVD, CD etc */
	message = g_strdup_printf (_("Additional media is required. Please insert the %s labeled '%s' to continue."), name, media_text);

	gpk_task->current_window = GTK_WINDOW (gtk_message_dialog_new (gpk_task->parent_window,
								   GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
								   /* TRANSLATORS: this is the window title when a new CD or DVD is required */
								   GTK_MESSAGE_INFO, GTK_BUTTONS_CANCEL, _("A media change is required")));
	gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG(gpk_task->current_window), "%s", message);

	/* TRANSLATORS: this is button text */
	gtk_dialog_add_button (GTK_DIALOG(gpk_task->current_window), _("Continue"), GTK_RESPONSE_YES);

	/* set icon name */
	gtk_window_set_icon_name (gpk_task->current_window, GPK_ICON_SOFTWARE_INSTALLER);

	g_signal_connect (gpk_task->current_window, "response", G_CALLBACK (gpk_task_dialog_response_cb), task);
	gtk_widget_show_all (GTK_WIDGET(gpk_task->current_window));
out:
	g_free (message);
	g_ptr_array_unref (array);
}

/**
 * gpk_task_add_dialog_deps_section:
 **/
static void
gpk_task_add_dialog_deps_section (PkTask *task,
				  GtkNotebook *tabbed_widget,
				  PkPackageSack *sack,
				  PkInfoEnum info)
{
	PkPackageSack *sack_tmp;
	GPtrArray *array_tmp = NULL;
	gboolean ret;
	GError *error = NULL;
	guint64 size;
	const gchar *title;
	GtkWidget *tab_page;
	GtkWidget *tab_label;

	sack_tmp = pk_package_sack_filter_by_info (sack, info);
	if (pk_package_sack_get_size (sack_tmp) == 0) {
		g_debug ("no packages with %s", pk_info_enum_to_string (info));
		goto out;
	}

	tab_page = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
	gtk_container_set_border_width (GTK_CONTAINER (tab_page), 12);

	/* get the header */
	switch (info) {
	case PK_INFO_ENUM_INSTALLING:
		/* TRANSLATORS: additional message text for the deps dialog */
		title = _("The following software also needs to be installed");
		tab_label = gtk_label_new (_("Install"));
		break;
	case PK_INFO_ENUM_REMOVING:
		/* TRANSLATORS: additional message text for the deps dialog */
		title = _("The following software also needs to be removed");
		tab_label = gtk_label_new (_("Remove"));
		break;
	case PK_INFO_ENUM_OBSOLETING:
		/* TRANSLATORS: additional message text for the deps dialog */
		title = _("The following software also needs to be removed");
		tab_label = gtk_label_new (_("Obsoleted"));
		break;
	case PK_INFO_ENUM_UPDATING:
		/* TRANSLATORS: additional message text for the deps dialog */
		title = _("The following software also needs to be updated");
		tab_label = gtk_label_new (_("Update"));
		break;
	case PK_INFO_ENUM_REINSTALLING:
		/* TRANSLATORS: additional message text for the deps dialog */
		title = _("The following software also needs to be re-installed");
		tab_label = gtk_label_new (_("Reinstall"));
		break;
	case PK_INFO_ENUM_DOWNGRADING:
		/* TRANSLATORS: additional message text for the deps dialog */
		title = _("The following software also needs to be downgraded");
		tab_label = gtk_label_new (_("Downgrade"));
		break;
	default:
		/* TRANSLATORS: additional message text for the deps dialog (we don't know how it's going to be processed -- eeek) */
		title = _("The following software also needs to be processed");
		tab_label = gtk_label_new (_("Other"));
		break;
	}

	/* get the size */
	ret = pk_package_sack_get_details (sack_tmp, NULL, &error);
	if (!ret) {
		g_warning ("failed to get details about packages: %s", error->message);
		g_error_free (error);
	}
	size = pk_package_sack_get_total_bytes (sack_tmp);

	/* embed title */
	array_tmp = pk_package_sack_get_array (sack_tmp);
	gpk_dialog_tabbed_download_size_widget (tab_page, title, size);
	gpk_dialog_tabbed_package_list_widget (tab_page, array_tmp);
	gtk_notebook_append_page (tabbed_widget, tab_page, tab_label);
out:
	if (array_tmp != NULL)
		g_ptr_array_unref (array_tmp);
	g_object_unref (sack_tmp);
}

/**
 * gpk_task_simulate_question:
 **/
static void
gpk_task_simulate_question (PkTask *task, guint request, PkResults *results)
{
	gboolean ret;
	GPtrArray *array = NULL;
	PkRoleEnum role;
	PkPackageSack *sack = NULL;
	guint inputs;
	const gchar *title;
	const gchar *message = NULL;
	GtkNotebook *tabbed_widget = NULL;
	PkBitfield transaction_flags = 0;

	GpkTask *gpk_task = GPK_TASK(task);

	/* save the current request */
	gpk_task->request = request;

	/* get data about the transaction */
	g_object_get (results,
		      "role", &role,
		      "inputs", &inputs,
		      "transaction-flags", &transaction_flags,
		      NULL);

	/* allow skipping of deps except when we remove other packages */
	if (role != PK_ROLE_ENUM_REMOVE_PACKAGES) {
		/* have we previously said we don't want to be shown the confirmation */
		ret = g_settings_get_boolean (gpk_task->settings, GPK_SETTINGS_SHOW_DEPENDS);
		if (!ret) {
			g_debug ("we've said we don't want the dep dialog");
			pk_task_user_accepted (PK_TASK(task), gpk_task->request);
			goto out;
		}
	}

	/* TRANSLATORS: title of a dependency dialog */
	title = _("Additional confirmation required");

	/* per-role messages */
	if (role == PK_ROLE_ENUM_INSTALL_PACKAGES) {

		/* TRANSLATORS: message text of a dependency dialog */
		message = ngettext ("To install this package, additional software also has to be modified.",
				    "To install these packages, additional software also has to be modified.", inputs);
	} else if (role == PK_ROLE_ENUM_REMOVE_PACKAGES) {

		/* TRANSLATORS: message text of a dependency dialog */
		message = ngettext ("To remove this package, additional software also has to be modified.",
				    "To remove these packages, additional software also has to be modified.", inputs);
	} else if (role == PK_ROLE_ENUM_UPDATE_PACKAGES) {

		/* TRANSLATORS: message text of a dependency dialog */
		message = ngettext ("To update this package, additional software also has to be modified.",
				    "To update these packages, additional software also has to be modified.", inputs);
	} else if (role == PK_ROLE_ENUM_INSTALL_FILES) {

		/* TRANSLATORS: message text of a dependency dialog */
		message = ngettext ("To install this file, additional software also has to be modified.",
				    "To install these files, additional software also has to be modified.", inputs);
	} else {

		/* TRANSLATORS: message text of a dependency dialog */
		message = _("To process this transaction, additional software also has to be modified.");
	}

	gpk_task->current_window = GTK_WINDOW (gtk_message_dialog_new (gpk_task->parent_window,
								   GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
								   GTK_MESSAGE_INFO, GTK_BUTTONS_CANCEL, "%s", title));
	gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (gpk_task->current_window), "%s", message);

	tabbed_widget = GTK_NOTEBOOK (gtk_notebook_new ());

	/* get the details for all the packages */
	sack = pk_results_get_package_sack (results);

	gpk_task_add_dialog_deps_section (task, tabbed_widget, sack,
					  PK_INFO_ENUM_INSTALLING);

	/* TRANSLATORS: additional message text for the deps dialog */
	gpk_task_add_dialog_deps_section (task, tabbed_widget, sack,
					  PK_INFO_ENUM_REMOVING);

	/* TRANSLATORS: additional message text for the deps dialog */
	gpk_task_add_dialog_deps_section (task, tabbed_widget, sack,
					  PK_INFO_ENUM_UPDATING);

	/* TRANSLATORS: additional message text for the deps dialog */
	gpk_task_add_dialog_deps_section (task, tabbed_widget, sack,
					  PK_INFO_ENUM_OBSOLETING);

	/* TRANSLATORS: additional message text for the deps dialog */
	gpk_task_add_dialog_deps_section (task, tabbed_widget, sack,
					  PK_INFO_ENUM_REINSTALLING);

	/* TRANSLATORS: additional message text for the deps dialog */
	gpk_task_add_dialog_deps_section (task, tabbed_widget, sack,
					  PK_INFO_ENUM_DOWNGRADING);

	gpk_dialog_embed_tabbed_widget (GTK_DIALOG(gpk_task->current_window),
					tabbed_widget);

	gpk_dialog_embed_do_not_show_widget (GTK_DIALOG(gpk_task->current_window), GPK_SETTINGS_SHOW_DEPENDS);
	/* TRANSLATORS: this is button text */
	gtk_dialog_add_button (GTK_DIALOG(gpk_task->current_window), _("Continue"), GTK_RESPONSE_YES);

	/* set icon name */
	gtk_window_set_icon_name (gpk_task->current_window, GPK_ICON_SOFTWARE_INSTALLER);

	g_signal_connect (gpk_task->current_window, "response", G_CALLBACK (gpk_task_dialog_response_cb), task);
	gtk_widget_show_all (GTK_WIDGET(gpk_task->current_window));
out:
	if (sack != NULL)
		g_object_unref (sack);
	if (array != NULL)
		g_ptr_array_unref (array);
}

/**
 * gpk_task_setup_dialog_untrusted:
 **/
static void
gpk_task_setup_dialog_untrusted (GpkTask *task)
{
	GtkWidget *widget;
	guint retval;
	GError *error = NULL;

	GpkTask *gpk_task = GPK_TASK(task);

	/* get UI */
	task->builder_untrusted = gtk_builder_new ();
	retval = gtk_builder_add_from_file (task->builder_untrusted, PKGDATADIR "/gpk-error.ui", &error);
	if (retval == 0) {
		g_warning ("failed to load ui: %s", error->message);
		g_error_free (error);
	}

	/* connect up default actions */
	widget = GTK_WIDGET (gtk_builder_get_object (gpk_task->builder_untrusted, "dialog_error"));
	g_signal_connect (widget, "delete_event", G_CALLBACK (gpk_task_button_decline_cb), task);

	/* set icon name */
	gtk_window_set_icon_name (GTK_WINDOW(widget), GPK_ICON_SOFTWARE_INSTALLER);

	/* connect up buttons */
	widget = GTK_WIDGET (gtk_builder_get_object (gpk_task->builder_untrusted, "button_close"));
	g_signal_connect (widget, "clicked", G_CALLBACK (gpk_task_button_decline_cb), task);

	/* don't show text in the expander */
	widget = GTK_WIDGET (gtk_builder_get_object (gpk_task->builder_untrusted, "expander_details"));
	gtk_widget_hide (widget);

	/* add to dialog */
	widget = GTK_WIDGET (gtk_builder_get_object (gpk_task->builder_untrusted, "button_force"));
	g_signal_connect (widget, "clicked", G_CALLBACK (gpk_task_button_accept_cb), task);
	gtk_widget_show (widget);
}

/**
 * gpk_task_setup_dialog_signature:
 **/
static void
gpk_task_setup_dialog_signature (GpkTask *task)
{
	GtkWidget *widget;
	guint retval;
	GError *error = NULL;

	/* get UI */
	task->builder_signature = gtk_builder_new ();
	retval = gtk_builder_add_from_file (task->builder_signature, PKGDATADIR "/gpk-signature.ui", &error);
	if (retval == 0) {
		g_warning ("failed to load ui: %s", error->message);
		g_error_free (error);
	}

	/* connect up default actions */
	widget = GTK_WIDGET (gtk_builder_get_object (task->builder_signature, "dialog_gpg"));
	g_signal_connect (widget, "delete_event", G_CALLBACK (gpk_task_button_decline_cb), task);

	/* set icon name */
	widget = GTK_WIDGET (gtk_builder_get_object (task->builder_signature, "dialog_gpg"));
	gtk_window_set_icon_name (GTK_WINDOW(widget), GPK_ICON_SOFTWARE_INSTALLER);

	/* connect up buttons */
	widget = GTK_WIDGET (gtk_builder_get_object (task->builder_signature, "button_yes"));
	g_signal_connect (widget, "clicked", G_CALLBACK (gpk_task_button_accept_cb), task);
	widget = GTK_WIDGET (gtk_builder_get_object (task->builder_signature, "button_no"));
	g_signal_connect (widget, "clicked", G_CALLBACK (gpk_task_button_decline_cb), task);
}

/**
 * gpk_task_setup_dialog_eula:
 **/
static void
gpk_task_setup_dialog_eula (GpkTask *task)
{
	GtkWidget *widget;
	guint retval;
	GError *error = NULL;

	/* get UI */
	task->builder_eula = gtk_builder_new ();
	retval = gtk_builder_add_from_file (task->builder_eula, PKGDATADIR "/gpk-eula.ui", &error);
	if (retval == 0) {
		g_warning ("failed to load ui: %s", error->message);
		g_error_free (error);
	}

	/* connect up default actions */
	widget = GTK_WIDGET (gtk_builder_get_object (task->builder_eula, "dialog_eula"));
	g_signal_connect (widget, "delete_event", G_CALLBACK (gpk_task_button_decline_cb), task);

	/* set icon name */
	widget = GTK_WIDGET (gtk_builder_get_object (task->builder_eula, "dialog_eula"));
	gtk_window_set_icon_name (GTK_WINDOW(widget), GPK_ICON_SOFTWARE_INSTALLER);

	/* connect up buttons */
	widget = GTK_WIDGET (gtk_builder_get_object (task->builder_eula, "button_agree"));
	g_signal_connect (widget, "clicked", G_CALLBACK (gpk_task_button_accept_cb), task);
	widget = GTK_WIDGET (gtk_builder_get_object (task->builder_eula, "button_cancel"));
	g_signal_connect (widget, "clicked", G_CALLBACK (gpk_task_button_decline_cb), task);
}

/**
 * gpk_task_class_init:
 **/
static void
gpk_task_class_init (GpkTaskClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	PkTaskClass *task_class = PK_TASK_CLASS (klass);

	object_class->finalize = gpk_task_finalize;
	task_class->untrusted_question = gpk_task_untrusted_question;
	task_class->key_question = gpk_task_key_question;
	task_class->eula_question = gpk_task_eula_question;
	task_class->media_change_question = gpk_task_media_change_question;
	task_class->simulate_question = gpk_task_simulate_question;
}

/**
 * gpk_task_init:
 * @task: This class instance
 **/
static void
gpk_task_init (GpkTask *task)
{
	task->request = 0;
	task->parent_window = NULL;
	task->current_window = NULL;
	task->settings = g_settings_new (GPK_SETTINGS_SCHEMA);

	/* setup dialogs ahead of time */
	gpk_task_setup_dialog_untrusted (task);
	gpk_task_setup_dialog_eula (task);
	gpk_task_setup_dialog_signature (task);
}

/**
 * gpk_task_finalize:
 * @object: The object to finalize
 **/
static void
gpk_task_finalize (GObject *object)
{
	GpkTask *task = GPK_TASK (object);

	g_object_unref (task->builder_untrusted);
	g_object_unref (task->builder_signature);
	g_object_unref (task->builder_eula);
	g_object_unref (task->settings);

	G_OBJECT_CLASS (gpk_task_parent_class)->finalize (object);
}

/**
 * gpk_task_new:
 *
 * Return value: a new GpkTask object.
 **/
GpkTask *
gpk_task_new (void)
{
	GpkTask *task;
	task = g_object_new (GPK_TYPE_TASK, NULL);
	return GPK_TASK (task);
}
