/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 Richard Hughes <richard@hughsie.com>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:gpk-client
 * @short_description: GObject class for libpackagekit-gnome client access
 *
 * A nice GObject to use for installing software in GNOME applications
 */

#include "config.h"

#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <pk-debug.h>
#include <pk-client.h>
#include <pk-package-id.h>
#include <pk-control.h>

#include "gpk-client.h"
#include "gpk-common.h"
#include "gpk-gnome.h"

static void     gpk_client_class_init	(GpkClientClass *klass);
static void     gpk_client_init		(GpkClient      *gclient);
static void     gpk_client_finalize	(GObject	*object);

#define GPK_CLIENT_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GPK_TYPE_CLIENT, GpkClientPrivate))

/**
 * GpkClientPrivate:
 *
 * Private #GpkClient data
 **/
struct _GpkClientPrivate
{
	PkClient		*client_action;
	PkClient		*client_resolve;
	GladeXML		*glade_xml;
	gint			 pulse_timeout;
	PkControl		*control;
	PkRoleEnum		 roles;
};

typedef enum {
	GPK_CLIENT_PAGE_PROGRESS,
	GPK_CLIENT_PAGE_CONFIRM,
	GPK_CLIENT_PAGE_ERROR,
	GPK_CLIENT_PAGE_LAST
} GpkClientPageEnum;

G_DEFINE_TYPE (GpkClient, gpk_client, G_TYPE_OBJECT)

/**
 * gpk_client_error_quark:
 *
 * Return value: Our personal error quark.
 **/
GQuark
gpk_client_error_quark (void)
{
	static GQuark quark = 0;
	if (!quark) {
		quark = g_quark_from_static_string ("gpk_client_error");
	}
	return quark;
}

/**
 * gpk_client_error_get_type:
 **/
#define ENUM_ENTRY(NAME, DESC) { NAME, "" #NAME "", DESC }
GType
gpk_client_error_get_type (void)
{
	static GType etype = 0;

	if (etype == 0) {
		static const GEnumValue values[] =
		{
			ENUM_ENTRY (GPK_CLIENT_ERROR_FAILED, "Failed"),
			{ 0, NULL, NULL }
		};
		etype = g_enum_register_static ("PkClientError", values);
	}
	return etype;
}

/**
 * gpk_client_set_page:
 **/
static void
gpk_client_set_page (GpkClient *gclient, GpkClientPageEnum page)
{
	GList *list, *l;
	GtkWidget *widget;
	guint i;

	g_return_if_fail (GPK_IS_CLIENT (gclient));

	widget = glade_xml_get_widget (gclient->priv->glade_xml, "hbox_hidden");
	list = gtk_container_get_children (GTK_CONTAINER (widget));
	for (l=list, i=0; l; l=l->next, i++) {
		if (i == page) {
			gtk_widget_show (l->data);
		} else {
			gtk_widget_hide (l->data);
		}
	}
}

/**
 * gpk_install_finished_timeout:
 **/
static gboolean
gpk_install_finished_timeout (gpointer data)
{
	gtk_main_quit ();
	return FALSE;
}

/**
 * gpk_client_finished_cb:
 **/
static void
gpk_client_finished_cb (PkClient *client, PkExitEnum exit, guint runtime, GpkClient *gclient)
{
	GtkWidget *widget;

	g_return_if_fail (GPK_IS_CLIENT (gclient));

	if (exit == PK_EXIT_ENUM_SUCCESS) {
		gpk_client_set_page (gclient, GPK_CLIENT_PAGE_CONFIRM);
		g_timeout_add_seconds (30, gpk_install_finished_timeout, gclient);
	}
	/* make insensitive */
	widget = glade_xml_get_widget (gclient->priv->glade_xml, "button_cancel");
	gtk_widget_set_sensitive (widget, FALSE);

	/* set to 100% */
	widget = glade_xml_get_widget (gclient->priv->glade_xml, "progressbar_percent");
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (widget), 1.0f);
}

/**
 * gpk_client_progress_changed_cb:
 **/
static void
gpk_client_progress_changed_cb (PkClient *client, guint percentage, guint subpercentage,
				guint elapsed, guint remaining, GpkClient *gclient)
{
	GtkWidget *widget;

	g_return_if_fail (GPK_IS_CLIENT (gclient));

	widget = glade_xml_get_widget (gclient->priv->glade_xml, "progressbar_percent");
	if (gclient->priv->pulse_timeout != 0) {
		g_source_remove (gclient->priv->pulse_timeout);
		gclient->priv->pulse_timeout = 0;
	}

	if (percentage != PK_CLIENT_PERCENTAGE_INVALID) {
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (widget), (gfloat) percentage / 100.0);
	}
}

/**
 * gpk_client_pulse_progress:
 **/
static gboolean
gpk_client_pulse_progress (GpkClient *gclient)
{
	GtkWidget *widget;

	g_return_val_if_fail (GPK_IS_CLIENT (gclient), FALSE);

	widget = glade_xml_get_widget (gclient->priv->glade_xml, "progressbar_percent");
	gtk_progress_bar_pulse (GTK_PROGRESS_BAR (widget));
	return TRUE;
}

/**
 * gpk_client_status_changed_cb:
 **/
static void
gpk_client_status_changed_cb (PkClient *client, PkStatusEnum status, GpkClient *gclient)
{
	GtkWidget *widget;
	gchar *text;

	g_return_if_fail (GPK_IS_CLIENT (gclient));

	widget = glade_xml_get_widget (gclient->priv->glade_xml, "progress_part_label");
	text = g_strdup_printf ("<b>%s</b>", gpk_status_enum_to_localised_text (status));
	gtk_label_set_markup (GTK_LABEL (widget), text);
	g_free (text);

	if (status == PK_STATUS_ENUM_WAIT) {
		if (gclient->priv->pulse_timeout == 0) {
			widget = glade_xml_get_widget (gclient->priv->glade_xml, "progressbar_percent");

			gtk_progress_bar_set_pulse_step (GTK_PROGRESS_BAR (widget ), 0.04);
			gclient->priv->pulse_timeout = g_timeout_add (75, (GSourceFunc) gpk_client_pulse_progress, gclient);
		}
	}
}

/**
 * gpk_client_error_code_cb:
 **/
static void
gpk_client_error_code_cb (PkClient *client, PkErrorCodeEnum code, const gchar *details, GpkClient *gclient)
{
	GtkWidget *widget;
	const gchar *title;
	gchar *title_bold;
	gchar *details_safe;

	g_return_if_fail (GPK_IS_CLIENT (gclient));

	gpk_client_set_page (gclient, GPK_CLIENT_PAGE_ERROR);

	/* set bold title */
	widget = glade_xml_get_widget (gclient->priv->glade_xml, "label_error_title");
	title = gpk_error_enum_to_localised_text (code);
	title_bold = g_strdup_printf ("<b>%s</b>", title);
	gtk_label_set_label (GTK_LABEL (widget), title_bold);
	g_free (title_bold);

	widget = glade_xml_get_widget (gclient->priv->glade_xml, "label_error_message");
	gtk_label_set_label (GTK_LABEL (widget), gpk_error_enum_to_localised_message (code));

	widget = glade_xml_get_widget (gclient->priv->glade_xml, "label_error_details");
	details_safe = g_markup_escape_text (details, -1);
	gtk_label_set_label (GTK_LABEL (widget), details_safe);
	g_free (details_safe);
}

/**
 * pk_client_package_cb:
 **/
static void
pk_client_package_cb (PkClient *client, PkInfoEnum info, const gchar *package_id,
		      const gchar *summary, GpkClient *gclient)
{
	gchar *text;
	GtkWidget *widget;

	g_return_if_fail (GPK_IS_CLIENT (gclient));

	text = gpk_package_id_pretty (package_id, summary);
	widget = glade_xml_get_widget (gclient->priv->glade_xml, "label_package");
	gtk_label_set_markup (GTK_LABEL (widget), text);
	g_free (text);
}

/**
 * pk_client_allow_cancel_cb:
 **/
static void
pk_client_allow_cancel_cb (PkClient *client, gboolean allow_cancel, GpkClient *gclient)
{
	GtkWidget *widget;

	g_return_if_fail (GPK_IS_CLIENT (gclient));

	widget = glade_xml_get_widget (gclient->priv->glade_xml, "button_cancel");
	gtk_widget_set_sensitive (widget, allow_cancel);
}

/**
 * gpk_client_window_delete_event_cb:
 **/
static gboolean
gpk_client_window_delete_event_cb (GtkWidget *widget, GdkEvent *event, GpkClient *gclient)
{
	g_return_val_if_fail (GPK_IS_CLIENT (gclient), FALSE);
	gtk_main_quit ();
	return FALSE;
}

/**
 * gpk_client_button_close_cb:
 **/
static void
gpk_client_button_close_cb (GtkWidget *widget, GpkClient *gclient)
{
	g_return_if_fail (GPK_IS_CLIENT (gclient));
	gtk_main_quit ();
}

/**
 * gpk_client_button_help_cb:
 **/
static void
gpk_client_button_help_cb (GtkWidget *widget, GpkClient *gclient)
{
	g_return_if_fail (GPK_IS_CLIENT (gclient));
	gpk_gnome_help (NULL);
}

/**
 * pk_client_button_cancel_cb:
 **/
static void
pk_client_button_cancel_cb (GtkWidget *widget, GpkClient *gclient)
{
	gboolean ret;
	GError *error = NULL;

	/* we might have a transaction running */
	ret = pk_client_cancel (gclient->priv->client_action, &error);
	if (!ret) {
		pk_warning ("failed to cancel client: %s", error->message);
		g_error_free (error);
	}
}

/**
 * gpk_client_error_message:
 **/
static void
gpk_client_error_msg (GpkClient *gclient, const gchar *title, const gchar *message)
{
	GtkWidget *widget;
	GtkWidget *dialog;

	/* hide the main window */
	widget = glade_xml_get_widget (gclient->priv->glade_xml, "window_updates");
	gtk_widget_hide (widget);

	dialog = gtk_message_dialog_new (GTK_WINDOW (widget), GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE, "%s", title);
	gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (dialog), "%s", message);
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

/**
 * gpk_client_error_set:
 *
 * Sets the correct error code (if allowed) and print to the screen
 * as a warning.
 **/
static gboolean
gpk_client_error_set (GError **error, gint code, const gchar *format, ...)
{
	va_list args;
	gchar *buffer = NULL;
	gboolean ret = TRUE;

	va_start (args, format);
	g_vasprintf (&buffer, format, args);
	va_end (args);

	/* dumb */
	if (error == NULL) {
		pk_warning ("No error set, so can't set: %s", buffer);
		ret = FALSE;
		goto out;
	}

	/* already set */
	if (*error != NULL) {
		pk_warning ("not NULL error!");
		g_clear_error (error);
	}

	/* propogate */
	g_set_error (error, GPK_CLIENT_ERROR, code, "%s", buffer);

out:
	g_free(buffer);
	return ret;
}

/**
 * gpk_client_install_local_file:
 * @gclient: a valid #GpkClient instance
 * @file_rel: a file such as <literal>./hal-devel-0.10.0.rpm</literal>
 * @error: a %GError to put the error code and message in, or %NULL
 *
 * Install a file locally, and get the deps from the repositories.
 * This is useful for double clicking on a .rpm or .deb file.
 *
 * Return value: %TRUE if the method succeeded
 **/
gboolean
gpk_client_install_local_file (GpkClient *gclient, const gchar *file_rel, GError **error)
{
	gboolean ret;
	GError *error_local = NULL;
	gchar *text;

	g_return_val_if_fail (GPK_IS_CLIENT (gclient), FALSE);
	g_return_val_if_fail (file_rel != NULL, FALSE);

	ret = pk_client_install_file (gclient->priv->client_action, file_rel, &error_local);
	if (!ret) {
		/* check if we got a permission denied */
		if (g_str_has_prefix (error_local->message, "org.freedesktop.packagekit.")) {
			gpk_client_error_msg (gclient, _("Failed to install file"),
					      _("You don't have the necessary privileges to install local files"));
			gpk_client_error_set (error, GPK_CLIENT_ERROR_FAILED, error_local->message);
		} else {
			text = g_markup_escape_text (error_local->message, -1);
			gpk_client_error_msg (gclient, _("Failed to install file"), text);
			g_free (text);
			gpk_client_error_set (error, GPK_CLIENT_ERROR_FAILED, error_local->message);
		}
		g_error_free (error_local);
		goto out;
	}

	/* wait for completion */
	gtk_main ();

	/* we're done */
	if (gclient->priv->pulse_timeout != 0) {
		g_source_remove (gclient->priv->pulse_timeout);
		gclient->priv->pulse_timeout = 0;
	}
out:
	return ret;
}

/**
 * gpk_client_install_package_id:
 * @gclient: a valid #GpkClient instance
 * @package_id: a package_id such as <literal>hal-info;0.20;i386;fedora</literal>
 * @error: a %GError to put the error code and message in, or %NULL
 *
 * Return value: %TRUE if the method succeeded
 **/
gboolean
gpk_client_install_package_id (GpkClient *gclient, const gchar *package_id, GError **error)
{
	GtkWidget *widget;
	GtkWidget *dialog;
	GtkResponseType button;
	gboolean ret;
	GError *error_local = NULL;
	gchar *text;
	guint len;
	guint i;
	GString *string;
	PkPackageItem *item;

	g_return_val_if_fail (GPK_IS_CLIENT (gclient), FALSE);
	g_return_val_if_fail (package_id != NULL, FALSE);

	/* are we dumb and can't check for depends? */
	if (!pk_enums_contain (gclient->priv->roles, PK_ROLE_ENUM_GET_DEPENDS)) {
		pk_warning ("skipping depends check");
		goto skip_checks;
	}

	/* reset */
	ret = pk_client_reset (gclient->priv->client_resolve, &error_local);
	if (!ret) {
		gpk_client_error_msg (gclient, _("Failed to reset client"), _("Failed to reset resolve"));
		gpk_client_error_set (error, GPK_CLIENT_ERROR_FAILED, error_local->message);
		ret = FALSE;
		goto out;
	}

	/* find out if this would drag in other packages */
	ret = pk_client_get_depends (gclient->priv->client_resolve, PK_FILTER_ENUM_NOT_INSTALLED, package_id, TRUE, &error_local);
	if (!ret) {
		text = g_strdup_printf ("%s: %s", _("Could not work out what packages would be also installed"), error_local->message);
		gpk_client_error_msg (gclient, _("Failed to get depends"), text);
		gpk_client_error_set (error, GPK_CLIENT_ERROR_FAILED, error_local->message);
		g_free (text);
		ret = FALSE;
		goto out;
	}

	/* any additional packages? */
	len = pk_client_package_buffer_get_size	(gclient->priv->client_resolve);
	if (len == 0) {
		pk_debug ("no additional deps");
		goto skip_checks;
	}

	/* process package list */
	string = g_string_new (_("The following packages also have to be downloaded:"));
	g_string_append (string, "\n\n");
	for (i=0; i<len; i++) {
		item = pk_client_package_buffer_get_item (gclient->priv->client_resolve, i);
		text = gpk_package_id_pretty_oneline (item->package_id, item->summary);
		g_string_append_printf (string, "%s\n", text);
		g_free (text);
	}
	/* remove last \n */
	g_string_set_size (string, string->len - 1);

	/* display messagebox  */
	text = g_string_free (string, FALSE);
	pk_debug ("text=%s", text);

	/* show UI */
	widget = glade_xml_get_widget (gclient->priv->glade_xml, "window_updates");
	dialog = gtk_message_dialog_new (GTK_WINDOW (widget), GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL,
					 "%s", _("Download additional packages?"));
	gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (dialog), "%s", text);
	button = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (GTK_WIDGET (dialog));
	g_free (text);

	/* did we click no or exit the window? */
	if (button != GTK_RESPONSE_OK) {
		gpk_client_error_msg (gclient, _("Failed to install package"), _("Additional packages were not downloaded"));
		gpk_client_error_set (error, GPK_CLIENT_ERROR_FAILED, error_local->message);
		ret = FALSE;
		goto out;
	}

skip_checks:
	/* try to install the package_id */
	ret = pk_client_install_package (gclient->priv->client_action, package_id, &error_local);
	if (!ret) {
		/* check if we got a permission denied */
		if (g_str_has_prefix (error_local->message, "org.freedesktop.packagekit.")) {
			gpk_client_error_msg (gclient, _("Failed to install package"),
					        _("You don't have the necessary privileges to install packages"));
			gpk_client_error_set (error, GPK_CLIENT_ERROR_FAILED, error_local->message);
		} else {
			text = g_markup_escape_text (error_local->message, -1);
			gpk_client_error_msg (gclient, _("Failed to install package"), text);
			gpk_client_error_set (error, GPK_CLIENT_ERROR_FAILED, error_local->message);
			g_free (text);
		}
		g_error_free (error_local);
		goto out;
	}

	/* wait for completion */
	gtk_main ();

	/* we're done */
	if (gclient->priv->pulse_timeout != 0) {
		g_source_remove (gclient->priv->pulse_timeout);
		gclient->priv->pulse_timeout = 0;
	}
out:
	return ret;
}

/**
 * gpk_client_install_package_name:
 * @gclient: a valid #GpkClient instance
 * @package: a pakage name such as <literal>hal-info</literal>
 * @error: a %GError to put the error code and message in, or %NULL
 *
 * Install a package of the newest and most correct version.
 *
 * Return value: %TRUE if the method succeeded
 **/
gboolean
gpk_client_install_package_name (GpkClient *gclient, const gchar *package, GError **error)
{
	gboolean ret;
	GError *error_local = NULL;
	guint len;
	guint i;
	gboolean already_installed = FALSE;
	gchar *package_id = NULL;
	PkPackageItem *item;

	g_return_val_if_fail (GPK_IS_CLIENT (gclient), FALSE);
	g_return_val_if_fail (package != NULL, FALSE);

	ret = pk_client_resolve (gclient->priv->client_resolve, PK_FILTER_ENUM_NONE, package, &error_local);
	if (!ret) {
		gpk_client_error_msg (gclient, _("Failed to resolve package"), _("Incorrect response from search"));
		gpk_client_error_set (error, GPK_CLIENT_ERROR_FAILED, error_local->message);
		ret = FALSE;
		goto out;
	}

	/* found nothing? */
	len = pk_client_package_buffer_get_size	(gclient->priv->client_resolve);
	if (len == 0) {
		gpk_client_error_msg (gclient, _("Failed to find package"), _("The package could not be found online"));
		gpk_client_error_set (error, GPK_CLIENT_ERROR_FAILED, error_local->message);
		ret = FALSE;
		goto out;
	}

	/* see what we've got already */
	for (i=0; i<len; i++) {
		item = pk_client_package_buffer_get_item (gclient->priv->client_resolve, i);
		if (item->info == PK_INFO_ENUM_INSTALLED) {
			already_installed = TRUE;
			break;
		} else if (item->info == PK_INFO_ENUM_AVAILABLE) {
			pk_debug ("package '%s' resolved", item->package_id);
			package_id = g_strdup (item->package_id);
			break;
		}
	}

	/* already installed? */
	if (already_installed) {
		gpk_client_error_msg (gclient, _("Failed to install package"), _("The package is already installed"));
		gpk_client_error_set (error, GPK_CLIENT_ERROR_FAILED, error_local->message);
		ret = FALSE;
		goto out;
	}

	/* got junk? */
	if (package_id == NULL) {
		gpk_client_error_msg (gclient, _("Failed to find package"), _("Incorrect response from search"));
		gpk_client_error_set (error, GPK_CLIENT_ERROR_FAILED, error_local->message);
		ret = FALSE;
		goto out;
	}

	/* install this specific package */
	ret = gpk_client_install_package_id (gclient, package_id, error);
out:
	g_free (package_id);
	return ret;
}

/**
 * gpk_client_install_provide_file:
 * @gclient: a valid #GpkClient instance
 * @full_path: a file path name such as <literal>/usr/sbin/packagekitd</literal>
 * @error: a %GError to put the error code and message in, or %NULL
 *
 * Install a package which provides a file on the system.
 *
 * Return value: %TRUE if the method succeeded
 **/
gboolean
gpk_client_install_provide_file (GpkClient *gclient, const gchar *full_path, GError **error)
{
	gboolean ret;
	GError *error_local = NULL;
	guint len;
	guint i;
	gboolean already_installed = FALSE;
	gchar *package_id = NULL;
	PkPackageItem *item;
	PkPackageId *ident;
	gchar *text;

	g_return_val_if_fail (GPK_IS_CLIENT (gclient), FALSE);
	g_return_val_if_fail (full_path != NULL, FALSE);

	ret = pk_client_search_file (gclient->priv->client_resolve, PK_FILTER_ENUM_NONE, full_path, &error_local);
	if (!ret) {
		text = g_strdup_printf ("%s: %s", _("Incorrect response from search"), error_local->message);
		gpk_client_error_msg (gclient, _("Failed to search for file"), text);
		gpk_client_error_set (error, GPK_CLIENT_ERROR_FAILED, error_local->message);
		g_free (text);
		ret = FALSE;
		goto out;
	}

	/* found nothing? */
	len = pk_client_package_buffer_get_size	(gclient->priv->client_resolve);
	if (len == 0) {
		gpk_client_error_msg (gclient, _("Failed to find package"), _("The file could not be found in any packages"));
		gpk_client_error_set (error, GPK_CLIENT_ERROR_FAILED, error_local->message);
		ret = FALSE;
		goto out;
	}

	/* see what we've got already */
	for (i=0; i<len; i++) {
		item = pk_client_package_buffer_get_item (gclient->priv->client_resolve, i);
		if (item->info == PK_INFO_ENUM_INSTALLED) {
			already_installed = TRUE;
			g_free (package_id);
			package_id = g_strdup (item->package_id);
			break;
		} else if (item->info == PK_INFO_ENUM_AVAILABLE) {
			pk_debug ("package '%s' resolved to:", item->package_id);
			package_id = g_strdup (item->package_id);
		}
	}

	/* already installed? */
	if (already_installed) {
		ident = pk_package_id_new_from_string (package_id);
		text = g_strdup_printf (_("The %s package already provides the file %s"), ident->name, full_path);
		gpk_client_error_msg (gclient, _("Failed to install file"), text);
		gpk_client_error_set (error, GPK_CLIENT_ERROR_FAILED, error_local->message);
		g_free (text);
		pk_package_id_free (ident);
		ret = FALSE;
		goto out;
	}

	/* got junk? */
	if (package_id == NULL) {
		gpk_client_error_msg (gclient, _("Failed to install file"), _("Incorrect response from file search"));
		gpk_client_error_set (error, GPK_CLIENT_ERROR_FAILED, error_local->message);
		ret = FALSE;
		goto out;
	}

	/* install this specific package */
	ret = gpk_client_install_package_id (gclient, package_id, error);
out:
	g_free (package_id);
	return ret;
}

/**
 * gpk_client_class_init:
 * @klass: The #GpkClientClass
 **/
static void
gpk_client_class_init (GpkClientClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gpk_client_finalize;
	g_type_class_add_private (klass, sizeof (GpkClientPrivate));
}

/**
 * gpk_client_init:
 * @gclient: a valid #GpkClient instance
 **/
static void
gpk_client_init (GpkClient *gclient)
{
	GtkWidget *widget;

	gclient->priv = GPK_CLIENT_GET_PRIVATE (gclient);

	gclient->priv->glade_xml = NULL;
	gclient->priv->pulse_timeout = 0;

	/* add application specific icons to search path */
	gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (),
					   PK_DATA G_DIR_SEPARATOR_S "icons");

	/* get actions */
	gclient->priv->control = pk_control_new ();
	gclient->priv->roles = pk_control_get_actions (gclient->priv->control);

	gclient->priv->client_action = pk_client_new ();
	g_signal_connect (gclient->priv->client_action, "finished",
			  G_CALLBACK (gpk_client_finished_cb), gclient);
	g_signal_connect (gclient->priv->client_action, "progress-changed",
			  G_CALLBACK (gpk_client_progress_changed_cb), gclient);
	g_signal_connect (gclient->priv->client_action, "status-changed",
			  G_CALLBACK (gpk_client_status_changed_cb), gclient);
	g_signal_connect (gclient->priv->client_action, "error-code",
			  G_CALLBACK (gpk_client_error_code_cb), gclient);
	g_signal_connect (gclient->priv->client_action, "package",
			  G_CALLBACK (pk_client_package_cb), gclient);
	g_signal_connect (gclient->priv->client_action, "allow-cancel",
			  G_CALLBACK (pk_client_allow_cancel_cb), gclient);

	gclient->priv->client_resolve = pk_client_new ();
	g_signal_connect (gclient->priv->client_resolve, "status-changed",
			  G_CALLBACK (gpk_client_status_changed_cb), gclient);
	pk_client_set_use_buffer (gclient->priv->client_resolve, TRUE, NULL);
	pk_client_set_synchronous (gclient->priv->client_resolve, TRUE, NULL);

	gclient->priv->glade_xml = glade_xml_new (PK_DATA "/gpk-install-file.glade", NULL, NULL);
	widget = glade_xml_get_widget (gclient->priv->glade_xml, "window_updates");

	/* Get the main window quit */
	g_signal_connect (widget, "delete_event",
			  G_CALLBACK (gpk_client_window_delete_event_cb), gclient);

	widget = glade_xml_get_widget (gclient->priv->glade_xml, "button_close");
	g_signal_connect (widget, "clicked",
			  G_CALLBACK (gpk_client_button_close_cb), gclient);
	widget = glade_xml_get_widget (gclient->priv->glade_xml, "button_close2");
	g_signal_connect (widget, "clicked",
			  G_CALLBACK (gpk_client_button_close_cb), gclient);
	widget = glade_xml_get_widget (gclient->priv->glade_xml, "button_close3");
	g_signal_connect (widget, "clicked",
			  G_CALLBACK (gpk_client_button_close_cb), gclient);

	widget = glade_xml_get_widget (gclient->priv->glade_xml, "button_cancel");
	g_signal_connect (widget, "clicked",
			  G_CALLBACK (pk_client_button_cancel_cb), gclient);
	gtk_widget_set_sensitive (widget, FALSE);

	widget = glade_xml_get_widget (gclient->priv->glade_xml, "button_help3");
	g_signal_connect (widget, "clicked",
			  G_CALLBACK (gpk_client_button_help_cb), gclient);
	widget = glade_xml_get_widget (gclient->priv->glade_xml, "button_help4");
	g_signal_connect (widget, "clicked",
			  G_CALLBACK (gpk_client_button_help_cb), gclient);
	widget = glade_xml_get_widget (gclient->priv->glade_xml, "button_help5");
	g_signal_connect (widget, "clicked",
			  G_CALLBACK (gpk_client_button_help_cb), gclient);

	/* set the label blank initially */
	widget = glade_xml_get_widget (gclient->priv->glade_xml, "progress_part_label");
	gtk_label_set_label (GTK_LABEL (widget), "");

	gpk_client_set_page (gclient, GPK_CLIENT_PAGE_PROGRESS);
}

/**
 * gpk_client_finalize:
 * @object: The object to finalize
 **/
static void
gpk_client_finalize (GObject *object)
{
	GpkClient *gclient;

	g_return_if_fail (GPK_IS_CLIENT (object));

	gclient = GPK_CLIENT (object);
	g_return_if_fail (gclient->priv != NULL);
	g_object_unref (gclient->priv->client_action);
	g_object_unref (gclient->priv->client_resolve);
	g_object_unref (gclient->priv->control);

	G_OBJECT_CLASS (gpk_client_parent_class)->finalize (object);
}

/**
 * gpk_client_new:
 *
 * PkClient is a nice GObject wrapper for gnome-packagekit and makes installing software easy
 *
 * Return value: A new %GpkClient instance
 **/
GpkClient *
gpk_client_new (void)
{
	GpkClient *gclient;
	gclient = g_object_new (GPK_TYPE_CLIENT, NULL);
	return GPK_CLIENT (gclient);
}

