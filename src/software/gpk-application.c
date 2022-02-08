/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007-2012 Richard Hughes <richard@hughsie.com>
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

#include <dbus/dbus-glib.h>
#include <errno.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>
#include <glib/gi18n.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <locale.h>
#include <math.h>
#include <packagekit-glib2/packagekit.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <common/gpk-common.h>
#include <common/gpk-dialog.h>
#include <common/gpk-enum.h>
#include <common/gpk-error.h>
#include <common/gpk-helper-run.h>
#include <common/gpk-task.h>
#include <common/gpk-debug.h>

#include "gpk-as-store.h"
#include "gpk-backend.h"
#include "gpk-categories.h"
#include "gpk-packages-list.h"

typedef enum {
	GPK_SEARCH_APP,
	GPK_SEARCH_PKGNAME,
	GPK_SEARCH_UNKNOWN
} GpkSearchType;

typedef enum {
	GPK_VIEW_SEARCH,
	GPK_VIEW_CATEGORIES,
	GPK_VIEW_UNKNOWN
} GpkPackageView;

typedef struct {
	GtkApplication		*application;

	GtkBuilder		*builder;

	GpkBackend		*backend;
	GCancellable		*cancellable;

	gboolean		 has_package;
	GtkListStore		*packages_store;

	gchar			*search_text;
	gboolean		 search_in_progress;
	GpkSearchType		 search_type;

	GpkPackageView		 package_view;
	gchar			*selection_id;

	GpkHelperRun		*helper_run;
	guint			 details_event_id;
	guint			 status_id;
	PkStatusEnum		 status_last;
	GSettings		*settings;
} GpkApplicationPrivate;

enum {
	GPK_STATE_INSTALLED,
	GPK_STATE_IN_LIST,
	GPK_STATE_COLLECTION,
	GPK_STATE_UNKNOWN
};

static void gpk_application_remove_packages_cb (PkTask *task, GAsyncResult *res, GpkApplicationPrivate *priv);
static void gpk_application_install_packages_cb (PkTask *task, GAsyncResult *res, GpkApplicationPrivate *priv);

static void gpk_application_perform_search (GpkApplicationPrivate *priv);
static void gpk_application_show_category (GpkApplicationPrivate *priv,
                                           const gchar           *category_id);
static void gpk_application_show_categories (GpkApplicationPrivate *priv);
static void gpk_application_restore_search (GpkApplicationPrivate *priv);

static gboolean
_g_strzero (const gchar *text)
{
	if (text == NULL)
		return TRUE;
	if (text[0] == '\0')
		return TRUE;
	return FALSE;
}

/**
 * gpk_application_state_get_icon:
 **/
static const gchar *
gpk_application_state_get_icon (PkBitfield state)
{
	if (state == 0)
		return gpk_info_enum_to_icon_name (PK_INFO_ENUM_AVAILABLE);

	if (state == pk_bitfield_value (GPK_STATE_INSTALLED))
		return gpk_info_enum_to_icon_name (PK_INFO_ENUM_INSTALLED);

	if (state == pk_bitfield_value (GPK_STATE_IN_LIST))
		return gpk_info_enum_to_icon_name (PK_INFO_ENUM_INSTALLING);

	if (state == pk_bitfield_from_enums (GPK_STATE_INSTALLED, GPK_STATE_IN_LIST, -1))
		return gpk_info_enum_to_icon_name (PK_INFO_ENUM_REMOVING);

	if (state == pk_bitfield_value (GPK_STATE_COLLECTION))
		return gpk_info_enum_to_icon_name (PK_INFO_ENUM_COLLECTION_AVAILABLE);

	if (state == pk_bitfield_from_enums (GPK_STATE_INSTALLED, GPK_STATE_COLLECTION, -1))
		return gpk_info_enum_to_icon_name (PK_INFO_ENUM_COLLECTION_INSTALLED);

	if (state == pk_bitfield_from_enums (GPK_STATE_IN_LIST, GPK_STATE_INSTALLED, GPK_STATE_COLLECTION, -1))
		return gpk_info_enum_to_icon_name (PK_INFO_ENUM_REMOVING); // need new icon

	if (state == pk_bitfield_from_enums (GPK_STATE_IN_LIST, GPK_STATE_COLLECTION, -1))
		return gpk_info_enum_to_icon_name (PK_INFO_ENUM_INSTALLING); // need new icon

	return NULL;
}

/**
 * gpk_application_set_text_buffer:
 **/
static void
gpk_application_set_text_buffer (GtkWidget *widget, const gchar *text)
{
	gchar *as_markup = NULL, *as_markup_scaped = NULL;

	if (_g_strzero (text))
		return;

	as_markup = as_markup_convert_simple (text, NULL);
	if (!as_markup)
		return;

	as_markup_scaped = g_markup_escape_text (as_markup, -1);

	gtk_label_set_markup (GTK_LABEL (widget), as_markup_scaped);

	g_free (as_markup);
	g_free (as_markup_scaped);
}

/**
 * gpk_application_get_selected_package:
 **/
static gboolean
gpk_application_get_selected_package (GpkApplicationPrivate *priv, gchar **package_id, gchar **summary)
{
	GtkTreeView *treeview;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	gboolean ret;

	/* get the selection and add */
	treeview = GTK_TREE_VIEW (gtk_builder_get_object (priv->builder, "treeview_packages"));
	selection = gtk_tree_view_get_selection (treeview);
	ret = gtk_tree_selection_get_selected (selection, &model, &iter);
	if (!ret) {
		g_warning ("no selection");
		goto out;
	}

	/* get data */
	if (summary == NULL) {
		gtk_tree_model_get (model, &iter,
				    PACKAGES_COLUMN_ID, package_id,
				    -1);
	} else {
		gtk_tree_model_get (model, &iter,
				    PACKAGES_COLUMN_ID, package_id,
				    PACKAGES_COLUMN_SUMMARY, summary,
				    -1);
	}
out:
	return ret;
}

/**
 * gpk_application_status_changed_timeout_cb:
 **/
static gboolean
gpk_application_status_changed_timeout_cb (GpkApplicationPrivate *priv)
{
	const gchar *text;
	GtkWidget *widget;

	/* set the text and show */
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "headerbar"));
	text = gpk_status_enum_to_localised_text (priv->status_last);
	gtk_header_bar_set_subtitle (GTK_HEADER_BAR(widget), text);

	/* show cancel button */
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "button_cancel"));
	gtk_widget_show (widget);

	/* show progressbar */
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "progressbar_progress"));
	gtk_widget_show (widget);

	/* never repeat */
	priv->status_id = 0;
	return FALSE;
}

/**
 * gpk_application_progress_cb:
 **/
static void
gpk_application_progress_cb (PkProgress *progress, PkProgressType type, GpkApplicationPrivate *priv)
{
	PkRoleEnum role;
	PkStatusEnum status;
	gint percentage;
	gboolean allow_cancel;
	GtkWidget *widget;

	g_object_get (progress,
		      "role", &role,
		      "status", &status,
		      "percentage", &percentage,
		      "allow-cancel", &allow_cancel,
		      NULL);

	if (type == PK_PROGRESS_TYPE_STATUS) {
		g_debug ("role now %s", pk_role_enum_to_string (role));
		g_debug ("status now %s", pk_status_enum_to_string (status));

		if (status == PK_STATUS_ENUM_FINISHED) {
			/* re-enable UI */
			widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "treeview_packages"));
			gtk_widget_set_sensitive (widget, TRUE);

			/* hide the cancel button */
			widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "button_cancel"));
			gtk_widget_hide (widget);

			/* we've not yet shown, so don't bother */
			if (priv->status_id > 0) {
				g_source_remove (priv->status_id);
				priv->status_id = 0;
			}

			widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "progressbar_progress"));
			gtk_widget_hide (widget);

			widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "headerbar"));
			gtk_header_bar_set_subtitle (GTK_HEADER_BAR(widget), NULL);

			goto out;
		}

		/* already pending show */
		if (priv->status_id > 0)
			goto out;

		/* only show after some time in the transaction */
		priv->status_id =
			g_timeout_add (GPK_UI_STATUS_SHOW_DELAY,
				       (GSourceFunc) gpk_application_status_changed_timeout_cb,
				       priv);
		g_source_set_name_by_id (priv->status_id,
					 "[GpkApplication] status-changed");

		/* save for the callback */
		priv->status_last = status;

	} else if (type == PK_PROGRESS_TYPE_PERCENTAGE) {
		widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "progressbar_progress"));
		if (percentage > 0) {
			gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (widget), (gfloat) percentage / 100.0f);
		} else {
			gtk_widget_hide (widget);
		}

	} else if (type == PK_PROGRESS_TYPE_ALLOW_CANCEL) {
		widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "button_cancel"));
		gtk_widget_set_sensitive (widget, allow_cancel);
	}
out:
	return;
}


/**
 * gpk_application_button_install_cb:
 **/
static void
gpk_application_button_install_cb (GtkAction *action, GpkApplicationPrivate *priv)
{
	GtkWidget *widget;
	gchar *package_id_selected = NULL, *summary_selected = NULL;
	gchar **package_ids = NULL;

	g_debug ("apply changes...");

	if (!gpk_application_get_selected_package (priv, &package_id_selected, &summary_selected)) {
		g_warning ("no package selected to install");
		return;
	}

	package_ids = g_new0 (gchar *, 2);
	package_ids[0] = g_strdup (package_id_selected);

	/* ensure new action succeeds */
	g_cancellable_reset (priv->cancellable);

	/* install */
	pk_task_install_packages_async (gpk_backend_get_task (priv->backend), package_ids, priv->cancellable,
	                                (PkProgressCallback) gpk_application_progress_cb, priv,
	                                (GAsyncReadyCallback) gpk_application_install_packages_cb, priv);

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "box_package_selection"));
	gtk_widget_hide (widget);

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "details_stack"));
	gtk_stack_set_visible_child_name (GTK_STACK (widget), "empty_page");

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "spinner_empty"));
	gtk_spinner_start (GTK_SPINNER (widget));

	g_free (package_id_selected);
	g_free (summary_selected);
	g_strfreev (package_ids);
}

/**
 * gpk_application_button_remove_cb:
 **/
static void
gpk_application_button_remove_cb (GtkAction *action, GpkApplicationPrivate *priv)
{
	GtkWidget *widget;
	gchar *package_id_selected = NULL, *summary_selected = NULL;
	gchar **package_ids = NULL;
	gboolean autoremove;

	g_debug ("apply changes...");

	if (!gpk_application_get_selected_package (priv, &package_id_selected, &summary_selected)) {
		g_warning ("no package selected to remove");
		return;
	}

	package_ids = g_new0 (gchar *, 2);
	package_ids[0] = g_strdup (package_id_selected);

	/* ensure new action succeeds */
	g_cancellable_reset (priv->cancellable);

	autoremove = g_settings_get_boolean (priv->settings, GPK_SETTINGS_ENABLE_AUTOREMOVE);

	/* remove */
	pk_task_remove_packages_async (gpk_backend_get_task (priv->backend), package_ids, TRUE, autoremove, priv->cancellable,
	                               (PkProgressCallback) gpk_application_progress_cb, priv,
	                               (GAsyncReadyCallback) gpk_application_remove_packages_cb, priv);

	/* hide details */
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "box_package_selection"));
	gtk_widget_hide (GTK_WIDGET(widget));

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "details_stack"));
	gtk_stack_set_visible_child_name (GTK_STACK (widget), "empty_page");

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "spinner_empty"));
	gtk_spinner_start (GTK_SPINNER (widget));

	g_free (package_id_selected);
	g_free (summary_selected);
	g_strfreev (package_ids);
}


/**
 * gpk_application_clear_packages:
 **/
static void
gpk_application_clear_packages (GpkApplicationPrivate *priv)
{
	/* clear existing array */
	priv->has_package = FALSE;
	gtk_list_store_clear (priv->packages_store);
}

/**
 * gpk_application_add_item_to_results:
 **/
static void
gpk_application_add_item_to_results (GpkApplicationPrivate *priv, PkPackage *item)
{
	AsComponent *component = NULL;
	GtkTreeIter iter;
	gchar *text;
	gboolean installed;
	PkBitfield state = 0;
	PkInfoEnum info;
	gchar *package_id = NULL;
	gchar *package_name = NULL;
	gchar *summary = NULL;

	/* get data */
	g_object_get (item,
		      "info", &info,
		      "package-id", &package_id,
		      "summary", &summary,
		      NULL);

	/* mark as got so we don't warn */
	priv->has_package = TRUE;

	/* are we in the package array? */
	installed = (info == PK_INFO_ENUM_INSTALLED) || (info == PK_INFO_ENUM_COLLECTION_INSTALLED);

	if (installed)
		pk_bitfield_add (state, GPK_STATE_INSTALLED);

	/* special icon */
	if (info == PK_INFO_ENUM_COLLECTION_INSTALLED || info == PK_INFO_ENUM_COLLECTION_AVAILABLE)
		pk_bitfield_add (state, GPK_STATE_COLLECTION);

	gtk_list_store_append (priv->packages_store, &iter);

	package_name = gpk_package_id_get_name (package_id);
	component = gpk_backend_get_component_by_pkgname (priv->backend, package_name);
	if (component) {
		text = gpk_common_format_details (as_component_get_name (component),
		                                  as_component_get_summary (component),
		                                  TRUE);
		gtk_list_store_set (priv->packages_store, &iter,
		                    PACKAGES_COLUMN_TEXT, text,
		                    PACKAGES_COLUMN_SUMMARY, summary,
		                    PACKAGES_COLUMN_STATE, state,
		                    PACKAGES_COLUMN_ID, package_id,
		                    PACKAGES_COLUMN_IMAGE, gpk_application_state_get_icon (state),
		                    PACKAGES_COLUMN_APP_NAME, as_component_get_name (component),
		                    -1);
	} else {
		text = gpk_package_id_format_details (package_id,
		                                      summary,
		                                      TRUE);

		gtk_list_store_set (priv->packages_store, &iter,
		                    PACKAGES_COLUMN_TEXT, text,
		                    PACKAGES_COLUMN_SUMMARY, summary,
		                    PACKAGES_COLUMN_STATE, state,
		                    PACKAGES_COLUMN_ID, package_id,
		                    PACKAGES_COLUMN_IMAGE, gpk_application_state_get_icon (state),
		                    PACKAGES_COLUMN_APP_NAME, NULL,
		                    -1);
	}

	g_free (package_id);
	g_free (package_name);
	g_free (summary);
	g_free (text);
}

/**
 * gpk_application_suggest_better_search:
 **/
static void
gpk_application_suggest_better_search (GpkApplicationPrivate *priv)
{
	const gchar *message = NULL;
	/* TRANSLATORS: no results were found for this search */
	const gchar *title = _("No results were found.");
	GtkTreeIter iter;
	gchar *text;
	PkBitfield state = 0;

	if (priv->search_type == GPK_SEARCH_APP) {
		/* TRANSLATORS: tell the user to switch to details search mode */
		message = _("Try searching package distributions by clicking the icon next to the search text.");
	} else {
		/* TRANSLATORS: tell the user to try harder */
		message = _("Try again with a different search term.");
	}

	text = g_strdup_printf ("%s\n%s", title, message);
	gtk_list_store_append (priv->packages_store, &iter);
	gtk_list_store_set (priv->packages_store, &iter,
			    PACKAGES_COLUMN_STATE, state,
			    PACKAGES_COLUMN_TEXT, text,
			    PACKAGES_COLUMN_IMAGE, "system-search",
			    PACKAGES_COLUMN_ID, NULL,
			    PACKAGES_COLUMN_APP_NAME, NULL,
			    -1);
	g_free (text);
}

/**
 * gpk_application_perform_search_idle_cb:
 **/
static gboolean
gpk_application_perform_search_idle_cb (GpkApplicationPrivate *priv)
{
	gpk_application_restore_search (priv);
	return FALSE;
}

/**
 * gpk_application_select_exact_match:
 *
 * NOTE: we have to do this in the finished_cb, as if we do this as we return
 * results we cancel the search and start getting the package details.
 **/
static void
gpk_application_select_exact_match (GpkApplicationPrivate *priv, const gchar *text)
{
	GtkTreeView *treeview;
	gboolean valid;
	GtkTreeIter iter;
	GtkTreePath *path;
	GtkTreeModel *model;
	GtkTreeSelection *selection = NULL;
	gchar *package_id;
	gchar **split;

	/* get the first iter in the array */
	treeview = GTK_TREE_VIEW (gtk_builder_get_object (priv->builder, "treeview_packages"));
	model = gtk_tree_view_get_model (treeview);
	valid = gtk_tree_model_get_iter_first (model, &iter);

	/* for all items in treeview */
	while (valid) {
		gtk_tree_model_get (model, &iter, PACKAGES_COLUMN_ID, &package_id, -1);
		if (package_id != NULL) {
			/* exact match, so select and scroll */
			split = pk_package_id_split (package_id);
			if (g_strcmp0 (split[PK_PACKAGE_ID_NAME], text) == 0) {
				selection = gtk_tree_view_get_selection (treeview);
				gtk_tree_selection_select_iter (selection, &iter);
				path = gtk_tree_model_get_path (model, &iter);
				gtk_tree_view_scroll_to_cell (treeview, path, NULL, FALSE, 0.5f, 0.5f);
				gtk_tree_path_free (path);
			}
			g_strfreev (split);

			/* no point continuing for a second match */
			if (selection != NULL)
				break;
		}
		g_free (package_id);
		valid = gtk_tree_model_iter_next (model, &iter);
	}

	if (!selection) {
		selection = gtk_tree_view_get_selection (treeview);
		gtk_tree_model_get_iter_first (model, &iter);
		gtk_tree_selection_select_iter (selection, &iter);

		path = gtk_tree_model_get_path (model, &iter);
		gtk_tree_view_scroll_to_cell (treeview, path, NULL, FALSE, 0.0f, 0.0f);
		gtk_tree_path_free (path);
	}
}

/**
 * gpk_application_run_installed:
 **/
static void
gpk_application_run_installed (GpkApplicationPrivate *priv, PkResults *results)
{
	guint i;
	GPtrArray *array;
	PkPackage *item;
	GPtrArray *package_ids_array;
	gchar **package_ids = NULL;
	PkInfoEnum info;
	gchar *package_id = NULL;

	/* get the package array and filter on INSTALLED */
	package_ids_array = g_ptr_array_new_with_free_func (g_free);
	array = pk_results_get_package_array (results);
	for (i=0; i<array->len; i++) {
		item = g_ptr_array_index (array, i);
		g_object_get (item,
			      "info", &info,
			      "package-id", &package_id,
			      NULL);
		if (info == PK_INFO_ENUM_INSTALLING)
			g_ptr_array_add (package_ids_array, g_strdup (package_id));
		g_free (package_id);
	}

	/* nothing to show */
	if (package_ids_array->len == 0) {
		g_debug ("nothing to do");
		goto out;
	}

	/* this is async */
	package_ids = pk_ptr_array_to_strv (package_ids_array);
	gpk_helper_run_show (priv->helper_run, package_ids);

out:
	g_strfreev (package_ids);
	g_ptr_array_unref (package_ids_array);
	g_ptr_array_unref (array);
}

/**
 * gpk_application_cancel_cb:
 **/
static void
gpk_application_cancel_cb (GtkWidget *button_widget, GpkApplicationPrivate *priv)
{
	g_cancellable_cancel (priv->cancellable);
}

/**
 * gpk_application_set_button_find_sensitivity:
 **/
static void
gpk_application_set_button_find_sensitivity (GpkApplicationPrivate *priv)
{
	GtkWidget *widget;

	/* only sensitive if not in the middle of a search */
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "entry_text"));
	gtk_widget_set_sensitive (widget, !priv->search_in_progress);
}

/**
 * gpk_application_show_search_entry:
 **/
static void
gpk_application_show_search_entry (GtkWidget *button_widget, GpkApplicationPrivate *priv)
{
	GtkWidget *widget;
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "stack_search_categories"));
	gtk_stack_set_visible_child_name (GTK_STACK (widget), "show_search");

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "entry_text"));
	gtk_entry_set_text (GTK_ENTRY(widget), "");
	gtk_widget_grab_focus (widget);
}

/**
 * gpk_application_show_categories_entry:
 **/
static void
gpk_application_show_categories_entry (GtkWidget *button_widget, GpkApplicationPrivate *priv)
{
	GtkWidget *widget;
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "stack_search_categories"));
	gtk_stack_set_visible_child_name (GTK_STACK (widget), "show_categories");

	gpk_application_show_categories (priv);
}


/**
 * gpk_application_search_cb:
 **/
static void
gpk_application_search_cb (PkClient *client, GAsyncResult *res, GpkApplicationPrivate *priv)
{
	PkResults *results;
	GError *error = NULL;
	PkError *error_code = NULL;
	GPtrArray *array = NULL;
	PkPackage *item;
	guint i;
	GtkWidget *widget;
	GtkWindow *window;

	/* get the results */
	results = pk_client_generic_finish (client, res, &error);
	if (results == NULL) {
		g_warning ("failed to search: %s", error->message);
		g_error_free (error);
		goto out;
	}

	/* check error code */
	error_code = pk_results_get_error_code (results);
	if (error_code != NULL) {
		g_warning ("failed to search: %s, %s", pk_error_enum_to_string (pk_error_get_code (error_code)), pk_error_get_details (error_code));

		/* if obvious message, don't tell the user */
		if (pk_error_get_code (error_code) != PK_ERROR_ENUM_TRANSACTION_CANCELLED) {
			window = GTK_WINDOW (gtk_builder_get_object (priv->builder, "window_manager"));
			gpk_error_dialog_modal (window, gpk_error_enum_to_localised_text (pk_error_get_code (error_code)),
						gpk_error_enum_to_localised_message (pk_error_get_code (error_code)), pk_error_get_details (error_code));
		}
		goto out;
	}

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "treeview_packages"));
	gtk_tree_view_set_model (GTK_TREE_VIEW (widget), NULL);

	/* get data */
	array = pk_results_get_package_array (results);
	for (i = 0; i < array->len; i++) {
		item = g_ptr_array_index (array, i);
		gpk_application_add_item_to_results (priv, item);
	}

	gtk_tree_view_set_model (GTK_TREE_VIEW (widget),
	                         GTK_TREE_MODEL (priv->packages_store));

	/* were there no entries found? */
	if (!priv->has_package)
		gpk_application_suggest_better_search (priv);

	/* if there is an exact match, select it */
	if (!_g_strzero (priv->selection_id)) {
		gpk_application_select_exact_match (priv, priv->selection_id);
	} else {
		gpk_application_select_exact_match (priv, priv->search_text);
	}

	/* focus back to the text extry */
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "entry_text"));
	gtk_widget_set_sensitive (widget, TRUE);
	gtk_widget_grab_focus (widget);

out:
	/* mark find button sensitive */
	priv->search_in_progress = FALSE;
	gpk_application_set_button_find_sensitivity (priv);

	if (error_code != NULL)
		g_object_unref (error_code);
	if (array != NULL)
		g_ptr_array_unref (array);
	if (results != NULL)
		g_object_unref (results);
}

static void
gpk_application_search_app (GpkApplicationPrivate *priv, gchar *search_text)
{
	gchar **packages = NULL;

	g_debug ("Searching appstream app: %s", search_text);

	packages = gpk_backend_search_pkgnames_with_component (priv->backend, search_text);
	pk_task_resolve_async (gpk_backend_get_task (priv->backend),
	                       pk_bitfield_from_enums (PK_FILTER_ENUM_NEWEST,
	                                               PK_FILTER_ENUM_ARCH,
	                                               -1),
	                       packages, priv->cancellable,
	                       (PkProgressCallback) gpk_application_progress_cb, priv,
	                       (GAsyncReadyCallback) gpk_application_search_cb, priv);

	g_strfreev (packages);
}

static void
gpk_application_search_categories (GpkApplicationPrivate *priv, gchar **categories)
{
	guint i = 0;
	gchar **packages = NULL;

	g_debug ("Searching appstream categories:");
	for (i = 0; categories[i] != NULL; i++) {
		g_debug (" - category: %s", categories[i]);
	}

	packages = gpk_backend_search_pkgnames_by_categories (priv->backend, categories);
	pk_task_resolve_async (gpk_backend_get_task (priv->backend),
	                       pk_bitfield_from_enums (PK_FILTER_ENUM_NEWEST,
	                                               PK_FILTER_ENUM_ARCH,
	                                               -1),
	                       packages, priv->cancellable,
	                       (PkProgressCallback) gpk_application_progress_cb, priv,
	                       (GAsyncReadyCallback) gpk_application_search_cb, priv);

	g_strfreev (packages);
}

static void
gpk_application_search_pkgname (GpkApplicationPrivate *priv, gchar *search_text)
{
	gchar **tokens = NULL;
	tokens = g_strsplit (search_text, " ", -1);

	pk_task_search_names_async (gpk_backend_get_task (priv->backend),
	                            pk_bitfield_from_enums (PK_FILTER_ENUM_NEWEST,
	                                                    PK_FILTER_ENUM_ARCH,
	                                                    -1),
	                            tokens, priv->cancellable,
	                            (PkProgressCallback) gpk_application_progress_cb, priv,
	                            (GAsyncReadyCallback) gpk_application_search_cb, priv);

	g_strfreev (tokens);
}

/**
 * gpk_application_perform_search:
 **/
static void
gpk_application_perform_search (GpkApplicationPrivate *priv)
{
	GtkEntry *entry;

	/*if we are in the middle of a search, just return */
	if (priv->search_in_progress == TRUE)
		return;

	g_debug ("CLEAR search");
	gpk_application_clear_packages (priv);

	entry = GTK_ENTRY (gtk_builder_get_object (priv->builder, "entry_text"));
	g_free (priv->search_text);
	priv->search_text = g_strdup (gtk_entry_get_text (entry));
	priv->package_view = GPK_VIEW_SEARCH;

	/* have we got input? */
	if (_g_strzero (priv->search_text)) {
		g_debug ("no input");
		return;
	}

	g_debug ("find %s", priv->search_text);

	/* mark find button insensitive */
	priv->search_in_progress = TRUE;
	gpk_application_set_button_find_sensitivity (priv);

	/* ensure new action succeeds */
	g_cancellable_reset (priv->cancellable);

	/* do the search */
	if (priv->search_type == GPK_SEARCH_APP) {
		gpk_application_search_app (priv, priv->search_text);
	} else if (priv->search_type == GPK_SEARCH_PKGNAME) {
		gpk_application_search_pkgname (priv, priv->search_text);
	} else {
		g_warning ("invalid search type");
	}
}

/**
 * gpk_application_restore_filter:
 **/
static void
gpk_application_restore_search (GpkApplicationPrivate *priv)
{
	gchar *restore_search = NULL;
	if (priv->package_view == GPK_VIEW_SEARCH) {
		gpk_application_perform_search (priv);
	} else if (priv->package_view == GPK_VIEW_CATEGORIES) {
		if (_g_strzero (priv->search_text)) {
			gpk_application_show_categories (priv);
		} else {
			restore_search = g_strdup(priv->search_text);
			gpk_application_show_category (priv, restore_search);
			g_free (restore_search);
		}
	}
}

/**
 * gpk_application_search_entry_activated:
 **/
static void
gpk_application_search_entry_activated (GtkWidget *button_widget, GpkApplicationPrivate *priv)
{
	gpk_application_perform_search (priv);
}

/**
 * gpk_application_quit:
 **/
static gboolean
gpk_application_quit (GpkApplicationPrivate *priv)
{
	GtkResponseType result;
	GtkWindow *window;
	GtkWidget *dialog;
	gint len = 0; // TODO

	/* do we have any items queued for removal or installation? */
	if (len != 0) {
		window = GTK_WINDOW (gtk_builder_get_object (priv->builder, "window_manager"));
		dialog = gtk_message_dialog_new (window, GTK_DIALOG_MODAL,
						 GTK_MESSAGE_WARNING, GTK_BUTTONS_CANCEL,
						 /* TRANSLATORS: title: warn the user they are quitting with unapplied changes */
						 "%s", _("Changes not applied"));
		gtk_dialog_add_button (GTK_DIALOG (dialog), _("Close _Anyway"), GTK_RESPONSE_OK);
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG(dialog),
							  "%s\n%s",
							  /* TRANSLATORS: tell the user the problem */
							  _("You have made changes that have not yet been applied."),
							  _("These changes will be lost if you close this window."));
		gtk_window_set_icon_name (GTK_WINDOW (dialog), GPK_ICON_SOFTWARE_INSTALLER);
		result = gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		/* did not agree */
		if (result != GTK_RESPONSE_OK)
			return FALSE;
	}

	/* we might have visual stuff running, close them down */
	g_cancellable_cancel (priv->cancellable);
	g_application_release (G_APPLICATION (priv->application));

	return TRUE;
}

/**
 * gpk_application_wm_close:
 * @event: The event type, unused.
 **/
static gboolean
gpk_application_wm_close (GtkWidget             *widget,
                          GdkEvent              *event,
                          GpkApplicationPrivate *priv)
{
	gpk_application_quit (priv);
	/* never destroy de main_window, since is managed by GtkApplication */
	return TRUE;
}

/**
 * gpk_application_text_changed_cb:
 **/
static gboolean
gpk_application_text_changed_cb (GtkEntry *entry, GpkApplicationPrivate *priv)
{
	/* mark find button sensitive */
	gpk_application_set_button_find_sensitivity (priv);
	return FALSE;
}

static void gpk_application_package_selection_changed_cb (GtkTreeSelection *selection, GpkApplicationPrivate *priv);

/**
 * gpk_application_install_packages_cb:
 **/
static void
gpk_application_install_packages_cb (PkTask *task, GAsyncResult *res, GpkApplicationPrivate *priv)
{
	GtkWidget *widget = NULL;
	PkResults *results;
	GError *error = NULL;
	PkError *error_code = NULL;
	GtkWindow *window;
	guint idle_id;

	g_debug ("installing packages...");

	/* get the results */
	results = pk_task_generic_finish (task, res, &error);
	if (results == NULL) {
		g_warning ("failed to install packages: %s", error->message);
		g_error_free (error);
		goto out;
	}

	/* check error code */
	error_code = pk_results_get_error_code (results);
	if (error_code != NULL) {
		g_warning ("failed to install packages: %s, %s", pk_error_enum_to_string (pk_error_get_code (error_code)), pk_error_get_details (error_code));

		/* if obvious message, don't tell the user */
		if (pk_error_get_code (error_code) != PK_ERROR_ENUM_TRANSACTION_CANCELLED) {
			window = GTK_WINDOW (gtk_builder_get_object (priv->builder, "window_manager"));
			gpk_error_dialog_modal (window, gpk_error_enum_to_localised_text (pk_error_get_code (error_code)),
						gpk_error_enum_to_localised_message (pk_error_get_code (error_code)), pk_error_get_details (error_code));
		}
		goto out;
	}

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "spinner_empty"));
	gtk_spinner_stop (GTK_SPINNER (widget));

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "box_package_selection"));
	gtk_widget_show (GTK_WIDGET(widget));

	/* idle add in the background */
	idle_id = g_idle_add ((GSourceFunc) gpk_application_perform_search_idle_cb, priv);
	g_source_set_name_by_id (idle_id, "[GpkApplication] search");

	/* find applications that were installed, and offer to run them */
	gpk_application_run_installed (priv, results);

out:
	if (error_code != NULL)
		g_object_unref (error_code);
	if (results != NULL)
		g_object_unref (results);
}

/**
 * gpk_application_remove_packages_cb:
 **/
static void
gpk_application_remove_packages_cb (PkTask *task, GAsyncResult *res, GpkApplicationPrivate *priv)
{
	GtkWidget *widget = NULL;
	PkResults *results;
	GError *error = NULL;
	PkError *error_code = NULL;
	GtkWindow *window;
	guint idle_id;

	g_debug ("removing packages...");

	/* get the results */
	results = pk_task_generic_finish (task, res, &error);
	if (results == NULL) {
		g_warning ("failed to remove packages: %s", error->message);
		g_error_free (error);
		goto out;
	}

	/* check error code */
	error_code = pk_results_get_error_code (results);
	if (error_code != NULL) {
		g_warning ("failed to remove packages: %s, %s", pk_error_enum_to_string (pk_error_get_code (error_code)), pk_error_get_details (error_code));

		/* if obvious message, don't tell the user */
		if (pk_error_get_code (error_code) != PK_ERROR_ENUM_TRANSACTION_CANCELLED) {
			window = GTK_WINDOW (gtk_builder_get_object (priv->builder, "window_manager"));
			gpk_error_dialog_modal (window, gpk_error_enum_to_localised_text (pk_error_get_code (error_code)),
						gpk_error_enum_to_localised_message (pk_error_get_code (error_code)), pk_error_get_details (error_code));
		}
		goto out;
	}

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "spinner_empty"));
	gtk_spinner_stop (GTK_SPINNER (widget));

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "box_package_selection"));
	gtk_widget_show (GTK_WIDGET(widget));

	/* idle add in the background */
	idle_id = g_idle_add ((GSourceFunc) gpk_application_perform_search_idle_cb, priv);
	g_source_set_name_by_id (idle_id, "[GpkApplication] search");

out:
	if (error_code != NULL)
		g_object_unref (error_code);
	if (results != NULL)
		g_object_unref (results);
}

static void
gpk_application_packages_add_columns (GpkApplicationPrivate *priv)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeView *treeview;

	treeview = GTK_TREE_VIEW (gtk_builder_get_object (priv->builder, "treeview_packages"));

	/* column for images */
	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_pixbuf_new ();
	g_object_set (renderer, "stock-size", GTK_ICON_SIZE_DIALOG, NULL);
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_add_attribute (column, renderer, "icon-name", PACKAGES_COLUMN_IMAGE);
	gtk_tree_view_append_column (treeview, column);

	/* column for name */
	renderer = gtk_cell_renderer_text_new ();
	/* TRANSLATORS: column for package name */
	column = gtk_tree_view_column_new_with_attributes (_("Name"), renderer,
							   "markup", PACKAGES_COLUMN_TEXT, NULL);
	gtk_tree_view_column_set_sort_column_id (column, PACKAGES_COLUMN_TEXT);
	gtk_tree_view_append_column (treeview, column);
}

/**
 * gpk_application_get_details_cb:
 **/
static void
gpk_application_get_details_cb (PkClient *client, GAsyncResult *res, GpkApplicationPrivate *priv)
{
	PkResults *results;
	AsComponent *component = NULL;
	GError *error = NULL;
	PkError *error_code = NULL;
	GPtrArray *array = NULL;
	PkDetails *item;
	GtkWidget *widget;
	gchar *value;
	const gchar *repo_name;
	gchar **split = NULL;
	GtkWindow *window;
	gchar *package_id = NULL;
	gchar *package_name = NULL;
	gchar *url = NULL;
	gchar *license = NULL;
	gchar *summary = NULL, *package_details = NULL, *package_pretty = NULL, *description = NULL, *escape_url = NULL;
	gchar *donation = NULL, *translate = NULL, *report = NULL;
	guint64 size;

	/* get the results */
	results = pk_client_generic_finish (client, res, &error);
	if (results == NULL) {
		g_warning ("failed to get list of details: %s", error->message);
		g_error_free (error);
		goto out;
	}

	/* check error code */
	error_code = pk_results_get_error_code (results);
	if (error_code != NULL) {
		g_warning ("failed to get details: %s, %s", pk_error_enum_to_string (pk_error_get_code (error_code)), pk_error_get_details (error_code));

		/* if obvious message, don't tell the user */
		if (pk_error_get_code (error_code) != PK_ERROR_ENUM_TRANSACTION_CANCELLED) {
			window = GTK_WINDOW (gtk_builder_get_object (priv->builder, "window_manager"));
			gpk_error_dialog_modal (window, gpk_error_enum_to_localised_text (pk_error_get_code (error_code)),
						gpk_error_enum_to_localised_message (pk_error_get_code (error_code)), pk_error_get_details (error_code));
		}
		goto out;
	}

	/* get data */
	array = pk_results_get_details_array (results);
	if (array->len != 1) {
		g_warning ("not one entry %u", array->len);
		goto out;
	}

	/* only choose the first item */
	item = g_ptr_array_index (array, 0);

	/* get data */
	g_object_get (item, "package-id", &package_id, NULL);
	split = pk_package_id_split (package_id);

	package_name = split[PK_PACKAGE_ID_NAME];
	component = gpk_backend_get_component_by_pkgname (priv->backend, package_name);
	if (component != NULL) {
		summary = g_strdup(as_component_get_name (component));
		package_details = g_strdup(as_component_get_summary (component));
		description = g_strdup(as_component_get_description (component));
		license = g_strdup(as_component_get_project_license(component));
		url = g_strdup(as_component_get_url (component, AS_URL_KIND_HOMEPAGE));
		donation = g_strdup(as_component_get_url (component, AS_URL_KIND_DONATION));
		translate = g_strdup(as_component_get_url (component, AS_URL_KIND_TRANSLATE));
		report = g_strdup(as_component_get_url (component, AS_URL_KIND_BUGTRACKER));
	}

	/* set the summary */
	if (!summary) {
		summary = g_strdup (_("Distribution package"));
	}
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "label_details_summary"));
	gtk_label_set_label (GTK_LABEL (widget), summary);

	/* set the package detail */
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "label_details_package_desc"));
	if (!package_details) {
		g_object_get (item, "summary", &package_details, NULL);
	}
	gtk_label_set_markup (GTK_LABEL (widget), package_details);

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "label_package"));
	package_pretty = gpk_package_id_format_pretty (package_id);
	gtk_label_set_markup (GTK_LABEL (widget), package_pretty);

	/* set the description */
	if (!description) {
		g_object_get (item,"description", &description, NULL);
	}
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "label_description"));
	gpk_application_set_text_buffer (widget, description);

	/* Show homepage */
	if (!url) {
		g_object_get (item, "url", &url, NULL);
	}
	if (url) {
		widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "label_homepage_title"));
		gtk_widget_set_visible (widget, TRUE);
		widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "label_homepage"));
		gtk_widget_set_visible (widget, TRUE);

		escape_url = g_markup_printf_escaped ("<a href=\"%s\" title=\"%s\">%s</a>", url, url, _("Visit the app's website"));
		gtk_label_set_markup (GTK_LABEL (widget), escape_url);
		g_free (escape_url);
	} else {
		widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "label_homepage_title"));
		gtk_widget_set_visible (widget, FALSE);
		widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "label_homepage"));
		gtk_widget_set_visible (widget, FALSE);
	}

	/* Show donation */
	if (donation) {
		widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "label_donate_title"));
		gtk_widget_set_visible (widget, TRUE);
		widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "label_donate"));
		gtk_widget_set_visible (widget, TRUE);

		escape_url = g_markup_printf_escaped ("<a href=\"%s\" title=\"%s\">%s</a>", donation, donation, _("Make a donation"));
		gtk_label_set_markup (GTK_LABEL (widget), escape_url);
		g_free (escape_url);
	} else {
		widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "label_donate_title"));
		gtk_widget_set_visible (widget, FALSE);
		widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "label_donate"));
		gtk_widget_set_visible (widget, FALSE);
	}

	/* Show bugtracking */
	if (report) {
		widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "label_report_title"));
		gtk_widget_set_visible (widget, TRUE);
		widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "label_report"));
		gtk_widget_set_visible (widget, TRUE);

		escape_url = g_markup_printf_escaped ("<a href=\"%s\" title=\"%s\">%s</a>", report, report, _("Report a problem"));
		gtk_label_set_markup (GTK_LABEL (widget), escape_url);
		g_free (escape_url);
	} else {
		widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "label_report_title"));
		gtk_widget_set_visible (widget, FALSE);
		widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "label_report"));
		gtk_widget_set_visible (widget, FALSE);
	}

	/* Show translations */
	if (translate) {
		widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "label_translate_title"));
		gtk_widget_set_visible (widget, TRUE);
		widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "label_translate"));
		gtk_widget_set_visible (widget, TRUE);

		escape_url = g_markup_printf_escaped ("<a href=\"%s\" title=\"%s\">%s</a>", translate, translate, _("Translate into your language"));
		gtk_label_set_markup (GTK_LABEL (widget), escape_url);
		g_free (escape_url);
	} else {
		widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "label_translate_title"));
		gtk_widget_set_visible (widget, FALSE);
		widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "label_translate"));
		gtk_widget_set_visible (widget, FALSE);
	}

	/* licence */
	if (!license) {
		g_object_get (item, "license", &license, NULL);
	}
	if (license != NULL) {
		widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "label_license_title"));
		gtk_widget_show (widget);
		widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "label_license"));
		gtk_label_set_label (GTK_LABEL (widget), license);
		gtk_label_set_line_wrap (GTK_LABEL (widget), TRUE);
		gtk_widget_show (widget);
	} else {
		widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "label_license_title"));
		gtk_widget_hide (widget);
		widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "label_license"));
		gtk_widget_hide (widget);
	}

	/* if non-zero, set the size */
	g_object_get (item, "size", &size, NULL);
	if (size > 0) {
		widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "label_size_title"));
		/* set the size */
		if (g_strcmp0 (split[PK_PACKAGE_ID_DATA], "meta") == 0) {
			/* TRANSLATORS: the size of the meta package */
			gtk_label_set_label (GTK_LABEL (widget), _("Size"));
		} else if (g_str_has_prefix (split[PK_PACKAGE_ID_DATA], "installed")) {
			/* TRANSLATORS: the installed size in bytes of the package */
			gtk_label_set_label (GTK_LABEL (widget), _("Installed size"));
		} else {
			/* TRANSLATORS: the download size of the package */
			gtk_label_set_label (GTK_LABEL (widget), _("Download size"));
		}
		gtk_widget_show (widget);
		widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "label_size"));
		value = g_format_size (size);
		gtk_label_set_label (GTK_LABEL (widget), value);
		gtk_widget_show (widget);
		g_free (value);
	} else {
		widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "label_size_title"));
		gtk_widget_hide (widget);
		widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "label_size"));
		gtk_widget_hide (widget);
	}

	/* set the repo text */
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "label_source"));
	/* get the full name of the repo from the repo_id */
	repo_name = gpk_backend_get_full_repo_name (priv->backend, split[PK_PACKAGE_ID_DATA]);
	gtk_label_set_label (GTK_LABEL (widget), repo_name);

out:
	g_free (package_id);
	g_free (url);
	g_free (license);
	g_free (summary);
	g_free (package_details);
	g_free (package_pretty);
	g_free (description);
	g_strfreev (split);
	if (error_code != NULL)
		g_object_unref (error_code);
	if (array != NULL)
		g_ptr_array_unref (array);
	if (results != NULL)
		g_object_unref (results);
}

/**
 * gpk_application_package_selection_changed_cb:
 **/
static void
gpk_application_package_selection_changed_cb (GtkTreeSelection *selection, GpkApplicationPrivate *priv)
{
	GtkWidget *widget;
	GtkTreeModel *model;
	GtkTreeIter iter;
	PkBitfield state;
	gchar **package_ids = NULL, **split = NULL;
	gchar *package_id = NULL, *summary = NULL;
	gboolean is_category = FALSE;

	/* ignore selection changed if we've just cleared the package list */
	if (!priv->has_package)
		return;

	/* This will only work in single or browse selection mode! */
	if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
		g_debug ("no row selected");

		widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "details_stack"));
		gtk_stack_set_visible_child_name (GTK_STACK (widget), "empty_page");
		goto out;
	}

	/* check we aren't a help line */
	gtk_tree_model_get (model, &iter,
	                    PACKAGES_COLUMN_STATE, &state,
	                    PACKAGES_COLUMN_ID, &package_id,
	                    PACKAGES_COLUMN_APP_NAME, &summary,
	                    PACKAGES_COLUMN_IS_CATEGORY, &is_category,
	                   -1);

	if (package_id == NULL) {
		g_debug ("ignoring help click");
		goto out;
	}

	if (is_category) {
		g_debug ("category %s selected...", package_id);
		gpk_application_show_category (priv, package_id);
		goto out;
	}

	/* Save selection to allow restore...*/
	g_free (priv->selection_id);

	split = pk_package_id_split (package_id);
	priv->selection_id = g_strdup (split[PK_PACKAGE_ID_NAME]);
	g_strfreev (split);

	/* show the menu item */
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "details_stack"));
	gtk_stack_set_visible_child_name (GTK_STACK (widget), "details_package");

	/* only show buttons if we are in the correct mode */

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "button_install"));
	gtk_widget_set_visible (widget, !pk_bitfield_contain (state, GPK_STATE_INSTALLED));

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "button_remove"));
	gtk_widget_set_visible (widget, pk_bitfield_contain (state, GPK_STATE_INSTALLED));

	/* clear the description text */
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "label_description"));
	gpk_application_set_text_buffer (widget, NULL);

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "icon_details_package"));
	gtk_image_set_from_icon_name (GTK_IMAGE (widget),
	                              gpk_application_state_get_icon (state),
	                              GTK_ICON_SIZE_DIALOG);

	/* ensure new action succeeds */
	g_cancellable_reset (priv->cancellable);

	/* get the details */
	package_ids = pk_package_ids_from_id (package_id);
	pk_client_get_details_async (PK_CLIENT(gpk_backend_get_task (priv->backend)),
				     package_ids, priv->cancellable,
				     (PkProgressCallback) gpk_application_progress_cb, priv,
				     (GAsyncReadyCallback) gpk_application_get_details_cb, priv);

out:
	g_free (package_id);
	g_free (summary);
	g_strfreev (package_ids);
}

/**
 * gpk_application_menu_search_by_pkgname:
 **/
static void
gpk_application_menu_search_by_pkgname (GtkMenuItem *item, GpkApplicationPrivate *priv)
{
	GtkWidget *widget;

	/* change type */
	priv->search_type = GPK_SEARCH_PKGNAME;
	g_debug ("set search type=%u", priv->search_type);

	/* save default to GSettings */
	g_settings_set_enum (priv->settings,
	                     GPK_SETTINGS_SEARCH_MODE,
	                     priv->search_type);

	/* set the new icon */
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "entry_text"));
	/* TRANSLATORS: entry tooltip: basic search by package name */
	gtk_widget_set_tooltip_text (widget, _("Searching for distribution packages"));
}

/**
 * gpk_application_menu_search_for_application:
 **/
static void
gpk_application_menu_search_for_application (GtkMenuItem *item, GpkApplicationPrivate *priv)
{
	GtkWidget *widget;

	/* set type */
	priv->search_type = GPK_SEARCH_APP;
	g_debug ("set search type=%u", priv->search_type);

	/* save default to GSettings */
	g_settings_set_enum (priv->settings,
	                     GPK_SETTINGS_SEARCH_MODE,
	                     priv->search_type);

	/* set the new icon */
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "entry_text"));
	/* TRANSLATORS: entry tooltip: applications search */
	gtk_widget_set_tooltip_text (widget, _("Searching for applications"));
}


/**
 * gpk_application_entry_text_icon_press_cb:
 **/
static void
gpk_application_entry_text_icon_press_cb (GtkEntry *entry, GtkEntryIconPosition icon_pos, GdkEventButton *event, GpkApplicationPrivate *priv)
{
	GtkMenu *menu = (GtkMenu*) gtk_menu_new ();
	GtkWidget *item;

	/* only respond to left button */
	if (event->button != 1)
		return;

	g_debug ("icon_pos=%u", icon_pos);

	/* TRANSLATORS: context menu item for the search type icon */
	item = gtk_menu_item_new_with_mnemonic (_("Search for apps"));
	g_signal_connect (G_OBJECT (item), "activate",
	                  G_CALLBACK (gpk_application_menu_search_for_application), priv);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

	/* TRANSLATORS: context menu item for the search type icon */
	item = gtk_menu_item_new_with_mnemonic (_("Search for distribution packages"));
	g_signal_connect (G_OBJECT (item), "activate",
	                  G_CALLBACK (gpk_application_menu_search_by_pkgname), priv);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

	gtk_widget_show_all (GTK_WIDGET (menu));
	gtk_menu_popup_at_widget(GTK_MENU (menu), gtk_widget_get_parent(GTK_WIDGET (entry)),
				GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST,
				(GdkEvent *)event);
}

/**
 * gpk_application_activate_about_cb:
 **/
static void
gpk_application_activate_about_cb (GSimpleAction *action,
				   GVariant *parameter,
				   gpointer user_data)
{
	GpkApplicationPrivate *priv = user_data;
	GtkWidget *main_window;
	const gchar copyright[] =
		"Copyright \xc2\xa9 2021 Matias De lellis\n"
		"Copyright \xc2\xa9 2007-2009 Richard Hughes";
	const char *authors[] = {
		"Matias De lellis <mati86dl@gmail.com>",
		"Richard Hughes <richard@hughsie.com>",
		NULL};
	const char *artists[] = {
		"Richard Hughes <richard@hughsie.com>",
		NULL};
	const char *license[] = {
		N_("Licensed under the GNU General Public License Version 2"),
		N_("PackageKit is free software; you can redistribute it and/or "
		   "modify it under the terms of the GNU General Public License "
		   "as published by the Free Software Foundation; either version 2 "
		   "of the License, or (at your option) any later version."),
		N_("PackageKit is distributed in the hope that it will be useful, "
		   "but WITHOUT ANY WARRANTY; without even the implied warranty of "
		   "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
		   "GNU General Public License for more details."),
		N_("You should have received a copy of the GNU General Public License "
		   "along with this program; if not, write to the Free Software "
		   "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA "
		   "02110-1301, USA.")
	};
	/* TRANSLATORS: put your own name here -- you deserve credit! */
	const char  *translators = _("translator-credits");
	char	    *license_trans;

	/* Translators comment: put your own name here to appear in the about dialog. */
	if (!strcmp (translators, "translator-credits")) {
		translators = NULL;
	}

	license_trans = g_strconcat (_(license[0]), "\n\n", _(license[1]), "\n\n",
				     _(license[2]), "\n\n", _(license[3]), "\n",  NULL);

	/* use parent */
	main_window = GTK_WIDGET (gtk_builder_get_object (priv->builder, "window_manager"));

	gtk_window_set_default_icon_name (GPK_ICON_SOFTWARE_INSTALLER);
	gtk_show_about_dialog (GTK_WINDOW (main_window),
	                       "program-name", _("Software"),
	                       "version", PACKAGE_VERSION,
	                       "copyright", copyright,
	                       "license", license_trans,
	                       "wrap-license", TRUE,
	                       "website-label", _("Xings Software Website"),
	                       "website", "https://github.com/matiasdelellis/xings-software",
	                       /* TRANSLATORS: description of NULL, gpk-application that is */
	                       "comments", _("Add or remove software installed on the system"),
	                       "authors", authors,
	                       "artists", artists,
	                       "translator-credits", translators,
	                       "logo-icon-name", GPK_ICON_SOFTWARE_INSTALLER,
	                       NULL);
	g_free (license_trans);
}

/**
 * gpk_application_activate_sources_cb:
 **/
static void
gpk_application_activate_sources_cb (GSimpleAction *action,
				     GVariant *parameter,
				     gpointer user_data)
{
	gboolean ret;
	gchar *command;
	GpkApplicationPrivate *priv = user_data;
	GtkWidget *window;
	guint xid;

	/* get xid */
	window = GTK_WIDGET (gtk_builder_get_object (priv->builder, "window_manager"));
	xid = gdk_x11_window_get_xid (gtk_widget_get_window (window));

	command = g_strdup_printf ("%s/xings-software-preferences --parent-window %u --startup-page sources", BINDIR, xid);
	g_debug ("running: %s", command);
	ret = g_spawn_command_line_async (command, NULL);
	if (!ret) {
		g_warning ("spawn of %s failed", command);
	}
	g_free (command);
}

/**
 * gpk_application_activate_log_cb:
 **/
static void
gpk_application_activate_log_cb (GSimpleAction *action,
				 GVariant *parameter,
				 gpointer user_data)
{
	gboolean ret;
	gchar *command;
	GpkApplicationPrivate *priv = user_data;
	GtkWidget *window;
	guint xid;

	/* get xid */
	window = GTK_WIDGET (gtk_builder_get_object (priv->builder, "window_manager"));
	xid = gdk_x11_window_get_xid (gtk_widget_get_window (window));

	command = g_strdup_printf ("%s/xings-software-history --parent-window %u", BINDIR, xid);
	g_debug ("running: %s", command);
	ret = g_spawn_command_line_async (command, NULL);
	if (!ret) {
		g_warning ("spawn of %s failed", command);
	}
	g_free (command);
}

/**
 * gpk_application_refresh_cache_cb:
 **/
static void
gpk_application_refresh_cache_cb (PkClient *client, GAsyncResult *res, GpkApplicationPrivate *priv)
{
	PkResults *results;
	GError *error = NULL;
	PkError *error_code = NULL;
	GtkWindow *window;

	/* get the results */
	results = pk_client_generic_finish (client, res, &error);
	if (results == NULL) {
		g_warning ("failed to refresh: %s", error->message);
		g_error_free (error);
		goto out;
	}

	/* check error code */
	error_code = pk_results_get_error_code (results);
	if (error_code != NULL) {
		g_warning ("failed to refresh: %s, %s", pk_error_enum_to_string (pk_error_get_code (error_code)), pk_error_get_details (error_code));

		/* if obvious message, don't tell the user */
		if (pk_error_get_code (error_code) != PK_ERROR_ENUM_TRANSACTION_CANCELLED) {
			window = GTK_WINDOW (gtk_builder_get_object (priv->builder, "window_manager"));
			gpk_error_dialog_modal (window, gpk_error_enum_to_localised_text (pk_error_get_code (error_code)),
						gpk_error_enum_to_localised_message (pk_error_get_code (error_code)), pk_error_get_details (error_code));
		}
		goto out;
	}
out:
	if (error_code != NULL)
		g_object_unref (error_code);
	if (results != NULL)
		g_object_unref (results);
}

/**
 * gpk_application_activate_refresh_cb:
 **/
static void
gpk_application_activate_refresh_cb (GSimpleAction *action,
				     GVariant *parameter,
				     gpointer user_data)
{
	GpkApplicationPrivate *priv = user_data;

	/* ensure new action succeeds */
	g_cancellable_reset (priv->cancellable);

	pk_task_refresh_cache_async (gpk_backend_get_task (priv->backend),
				     TRUE, priv->cancellable,
				     (PkProgressCallback) gpk_application_progress_cb, priv,
				     (GAsyncReadyCallback) gpk_application_refresh_cache_cb, priv);
}

/**
 * gpk_application_show_category
 */
static void
gpk_application_show_category (GpkApplicationPrivate *priv,
                               const gchar           *category_id)
{
	GtkWidget *widget = NULL;
	GpkCategory *category = NULL;
	gchar **categories;
	gchar *category_name = NULL;

	gpk_application_clear_packages (priv);

	category = gpk_backend_get_category_by_id (priv->backend, category_id);
	category_name = gpk_category_get_name (category);

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "label_category"));
	gtk_label_set_label (GTK_LABEL (widget), category_name);

	g_free (priv->search_text);
	priv->search_text = g_strdup (category_id);

	categories = gpk_category_get_categories (category);
	gpk_application_search_categories (priv, categories);

	g_strfreev (categories);
	g_free (category_name);
}

/**
 * gpk_application_show_categories:
 **/
static void
gpk_application_show_categories (GpkApplicationPrivate *priv)
{
	GtkWidget *widget = NULL;
	GPtrArray *categories = NULL;
	GpkCategory *category = NULL;
	GtkTreeIter iter;
	gchar *id = NULL, *name = NULL, *comment = NULL, *icon = NULL;
	gchar *text = NULL;
	guint i = 0;

	g_debug ("Show categories...");

	gpk_application_clear_packages (priv);

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "label_category"));
	gtk_label_set_label (GTK_LABEL (widget), _("Categories"));

	priv->package_view = GPK_VIEW_CATEGORIES;
	g_free (priv->search_text);
	priv->search_text = NULL;

	categories = gpk_backend_get_principals_categories (priv->backend);
	for (i = 0; i < categories->len; i++) {
		category = GPK_CATEGORY (g_ptr_array_index (categories, i));
		id = gpk_category_get_id (category);
		name = gpk_category_get_name (category);
		comment = gpk_category_get_comment (category);
		text = gpk_common_format_details (name, comment, TRUE);
		icon = gpk_category_get_icon (category);

		gtk_list_store_append (priv->packages_store, &iter);
		gtk_list_store_set (priv->packages_store, &iter,
		                    PACKAGES_COLUMN_TEXT, text,
		                    PACKAGES_COLUMN_IMAGE, icon,
		                    PACKAGES_COLUMN_ID, id,
		                    PACKAGES_COLUMN_APP_NAME, name,
		                    PACKAGES_COLUMN_IS_CATEGORY, TRUE,
		                    -1);

		g_free (id);
		g_free (name);
		g_free (comment);
		g_free (icon);
		g_free (text);
	}

	priv->has_package = TRUE;
}

/*
 * gpk_application_key_changed_cb:
 *
 * We might have to do things when the keys change; do them here.
 **/
static void
gpk_application_key_changed_cb (GSettings *settings, const gchar *key, GpkApplicationPrivate *priv)
{
	// TODO:
}

/**
 * gpk_application_open_backend_ready:
 **/
static void
gpk_application_open_backend_ready (GpkBackend            *backend,
                                    GAsyncResult          *res,
                                    GpkApplicationPrivate *priv)
{
	GtkWidget *widget = NULL;
	GError *error = NULL;

	if (!gpk_backend_open_finish (backend, res, &error)) {
		g_critical ("Failed to open backend: %s", error->message);
		widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "window_manager"));
		gpk_error_dialog_modal (GTK_WINDOW (widget),
		                        _("Software"),
		                        _("There was an error starting the application"),
		                        error->message);
		g_clear_error (&error);
		gpk_application_quit (priv);
		return;
	}

	/* finally open main gui */
	gpk_application_show_categories (priv);

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "spinner_empty"));
	gtk_spinner_stop (GTK_SPINNER (widget));

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "progressbar_progress"));
	gtk_widget_hide (widget);

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "box_package_selection"));
	gtk_widget_show (GTK_WIDGET(widget));
}

/**
 * gpk_application_activate_cb:
 **/
static void
gpk_application_activate_cb (GtkApplication *_application, GpkApplicationPrivate *priv)
{
	GtkWindow *window;
	window = GTK_WINDOW (gtk_builder_get_object (priv->builder, "window_manager"));
	gtk_window_present (window);
}


/**
 * gpk_application_startup_cb:
 **/
static void
gpk_application_startup_cb (GtkApplication *application, GpkApplicationPrivate *priv)
{
	GError *error = NULL;
	GMenuModel *menu;
	GtkTreeSelection *selection;
	GtkWidget *main_window;
	GtkWidget *widget;
	guint retval;

	priv->settings = g_settings_new (GPK_SETTINGS_SCHEMA);
	priv->cancellable = g_cancellable_new ();

	priv->backend = gpk_backend_new ();

	/* watch gnome-packagekit keys */
	g_signal_connect (priv->settings, "changed", G_CALLBACK (gpk_application_key_changed_cb), priv);

	/* create array stores */

	priv->packages_store = gpk_packages_list_store_new ();

	/* add application specific icons to search path */
	gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (),
					   PKGDATADIR G_DIR_SEPARATOR_S "icons");
	gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (),
					   "/usr/share/PackageKit/icons");

	/* get UI */
	priv->builder = gtk_builder_new ();
	retval = gtk_builder_add_from_file (priv->builder, PKGDATADIR "/gpk-application.ui", &error);
	if (retval == 0) {
		g_warning ("failed to load ui: %s", error->message);
		g_error_free (error);
		return;
	}

	main_window = GTK_WIDGET (gtk_builder_get_object (priv->builder, "window_manager"));

	gtk_application_add_window (application, GTK_WINDOW (main_window));
	gtk_window_set_application (GTK_WINDOW (main_window), application);

	/* prvent close by windows manager */
	g_signal_connect (G_OBJECT(main_window), "delete_event",
	                  G_CALLBACK(gpk_application_wm_close), priv);

	/* setup the application menu */
	menu = G_MENU_MODEL (gtk_builder_get_object (priv->builder, "appmenu"));
	gtk_application_set_app_menu (priv->application, menu);

	/* helpers */
	priv->helper_run = gpk_helper_run_new ();
	gpk_helper_run_set_parent (priv->helper_run, GTK_WINDOW (main_window));

	/* Hide window first so that the dialogue resizes itself without redrawing */
	gtk_widget_hide (main_window);
	gtk_window_set_icon_name (GTK_WINDOW (main_window), GPK_ICON_SOFTWARE_INSTALLER);
	gtk_window_set_default_icon_name (GPK_ICON_SOFTWARE_INSTALLER);

	/* install */
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "button_install"));
	g_signal_connect (widget, "clicked",
			  G_CALLBACK (gpk_application_button_install_cb), priv);

	/* remove */
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "button_remove"));
	g_signal_connect (widget, "clicked",
			  G_CALLBACK (gpk_application_button_remove_cb), priv);

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "details_stack"));
	gtk_stack_set_visible_child_name (GTK_STACK (widget), "empty_page");

	/* search cancel button */
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "button_cancel"));
	g_signal_connect (widget, "clicked",
			  G_CALLBACK (gpk_application_cancel_cb), priv);
	gtk_widget_hide (widget);

	// Categories
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "button_categories"));
	g_signal_connect (widget, "clicked",
	                  G_CALLBACK (gpk_application_show_search_entry), priv);

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "button_categories_home"));
	g_signal_connect (widget, "clicked",
	                  G_CALLBACK (gpk_application_show_categories_entry), priv);

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "button_search_home"));
	g_signal_connect (widget, "clicked",
	                  G_CALLBACK (gpk_application_show_categories_entry), priv);

	/* the fancy text entry widget */
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "entry_text"));

	g_signal_connect (widget, "activate",
			  G_CALLBACK (gpk_application_search_entry_activated), priv);
	g_signal_connect (widget, "icon-press",
			  G_CALLBACK (gpk_application_entry_text_icon_press_cb), priv);

	g_signal_connect (GTK_EDITABLE (widget), "changed",
			  G_CALLBACK (gpk_application_text_changed_cb), priv);

	/* mark find button insensitive */
	gpk_application_set_button_find_sensitivity (priv);

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "treeview_packages"));
	gtk_tree_view_columns_autosize (GTK_TREE_VIEW (widget));

	/* create package tree view */
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "treeview_packages"));
	gtk_tree_view_set_model (GTK_TREE_VIEW (widget),
				 GTK_TREE_MODEL (priv->packages_store));

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
	g_signal_connect (selection, "changed",
			  G_CALLBACK (gpk_application_package_selection_changed_cb), priv);

	/* add columns to the tree view */
	gpk_application_packages_add_columns (priv);

	/* Show empty page */
	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "box_package_selection"));
	gtk_widget_hide (GTK_WIDGET(widget));

	widget = GTK_WIDGET (gtk_builder_get_object (priv->builder, "spinner_empty"));
	gtk_spinner_start (GTK_SPINNER (widget));

	/* set a size, as much as the screen allows */
	gtk_window_set_default_size (GTK_WINDOW (main_window), 1000, 600);
	gtk_window_set_position (GTK_WINDOW (main_window), GTK_WIN_POS_CENTER);
	gtk_widget_show (GTK_WIDGET(main_window));

	/* configure app */
	priv->search_type = g_settings_get_enum (priv->settings, GPK_SETTINGS_SEARCH_MODE);
	if (priv->search_type == GPK_SEARCH_APP) {
		gpk_application_menu_search_for_application (NULL, priv);
	} else if (priv->search_type == GPK_SEARCH_PKGNAME) {
		gpk_application_menu_search_by_pkgname (NULL, priv);
	} else {
		/* mode not recognized */
		g_warning ("cannot recognize mode %u, using name", priv->search_type);
		gpk_application_menu_search_by_pkgname (NULL, priv);
	}

	/* Open backend and wait... */
	gpk_backend_open (priv->backend,
	                  priv->cancellable,
	                  (GAsyncReadyCallback)  gpk_application_open_backend_ready,
	                  priv);

}

static void
gpk_application_activate_quit_cb (GSimpleAction *action,
				  GVariant *parameter,
				  gpointer user_data)
{
	GpkApplicationPrivate *priv = user_data;
	gpk_application_quit (priv);
}

static GActionEntry gpk_menu_app_entries[] = {
	{ "sources",		gpk_application_activate_sources_cb, NULL, NULL, NULL },
	{ "refresh",		gpk_application_activate_refresh_cb, NULL, NULL, NULL },
	{ "log",		gpk_application_activate_log_cb, NULL, NULL, NULL },
	{ "quit",		gpk_application_activate_quit_cb, NULL, NULL, NULL },
	{ "about",		gpk_application_activate_about_cb, NULL, NULL, NULL },
};

/**
 * main:
 **/
int
main (int argc, char *argv[])
{
	gboolean program_version = FALSE;
	GOptionContext *context;
	gboolean ret;
	gint status = 0;
	GpkApplicationPrivate *priv;

	const GOptionEntry options[] = {
		{ "version", '\0', 0, G_OPTION_ARG_NONE, &program_version,
		  /* TRANSLATORS: show the program version */
		  _("Show the program version and exit"), NULL },
		{ NULL}
	};

	setlocale (LC_ALL, "");

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	dbus_g_thread_init ();
	gtk_init (&argc, &argv);

	context = g_option_context_new (NULL);
	g_option_context_set_summary (context, _("Install Software"));
	g_option_context_add_main_entries (context, options, NULL);
	g_option_context_add_group (context, gpk_debug_get_option_group ());
	g_option_context_add_group (context, gtk_get_option_group (TRUE));
	g_option_context_parse (context, &argc, &argv, NULL);
	g_option_context_free (context);

	/* add PackageKit */
	gpk_debug_add_log_domain ("PackageKit");

	if (program_version) {
		g_print (VERSION "\n");
		return 0;
	}

	/* are we running privileged */
	ret = gpk_check_privileged_user (_("Software installer"), TRUE);
	if (!ret)
		return 1;

	priv = g_new0 (GpkApplicationPrivate, 1);

	/* are we already activated? */
	priv->application = gtk_application_new ("org.xings.Software", 0);
	g_signal_connect (priv->application, "startup",
			  G_CALLBACK (gpk_application_startup_cb), priv);
	g_signal_connect (priv->application, "activate",
			  G_CALLBACK (gpk_application_activate_cb), priv);
	g_action_map_add_action_entries (G_ACTION_MAP (priv->application),
					 gpk_menu_app_entries,
					 G_N_ELEMENTS (gpk_menu_app_entries),
					 priv);

	/* run */
	status = g_application_run (G_APPLICATION (priv->application), argc, argv);
	g_object_unref (priv->application);

	if (priv->details_event_id > 0)
		g_source_remove (priv->details_event_id);

	if (priv->backend != NULL)
		g_object_unref (priv->backend);
	if (priv->packages_store != NULL)
		g_object_unref (priv->packages_store);
	if (priv->settings != NULL)
		g_object_unref (priv->settings);
	if (priv->builder != NULL)
		g_object_unref (priv->builder);
	if (priv->helper_run != NULL)
		g_object_unref (priv->helper_run);
	if (priv->cancellable != NULL)
		g_object_unref (priv->cancellable);
	if (priv->status_id > 0)
		g_source_remove (priv->status_id);

	g_free (priv->search_text);
	g_free (priv);

	return status;
}
