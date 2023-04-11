/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2022-2023 Matias De lellis <mati86dl@gmail.com>
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
#include <gio/gio.h>

#include "gpk-packages-list.h"

#define A_BEFORE_B -1
#define A_WITH_B 0
#define A_AFTER_B 1

static gint
gpk_packages_list_column_sort_func (GtkTreeModel *model,
                                    GtkTreeIter  *a,
                                    GtkTreeIter  *b,
                                    gpointer      user_data)
{
	gchar *app_name_a, *app_name_b;
	gchar *package_id_a, *package_id_b;
	gboolean is_special_a, is_special_b;
	gboolean is_separator_a, is_separator_b;
	gboolean is_category_a, is_category_b;
	gint result = 0;

	gtk_tree_model_get (model, a,
	                    PACKAGES_COLUMN_APP_NAME, &app_name_a,
	                    PACKAGES_COLUMN_ID, &package_id_a,
	                    PACKAGES_COLUMN_IS_SPECIAL, &is_special_a,
	                    PACKAGES_COLUMN_IS_SEPARATOR, &is_separator_a,
	                    PACKAGES_COLUMN_IS_CATEGORY, &is_category_a,
	                    -1);
	gtk_tree_model_get (model, b,
	                    PACKAGES_COLUMN_APP_NAME, &app_name_b,
	                    PACKAGES_COLUMN_ID, &package_id_b,
	                    PACKAGES_COLUMN_IS_SPECIAL, &is_special_b,
	                    PACKAGES_COLUMN_IS_SEPARATOR, &is_separator_b,
	                    PACKAGES_COLUMN_IS_CATEGORY, &is_category_b,
	                    -1);

	// first specials
	if (is_special_a && is_special_b) {
		result = strcasecmp (app_name_a, app_name_b);
	} else if (is_special_a) {
		result = A_BEFORE_B;
	} else if (is_special_b) {
		result = A_AFTER_B;
	// then separator
	} else if (is_separator_a && is_separator_b) {
		result = A_WITH_B;
	} else if (is_separator_a && is_special_b) {
		result = A_AFTER_B;
	} else if (is_separator_a && is_category_b) {
		result = A_BEFORE_B;
	// then categories
	} else if (is_category_a && is_category_b) {
		result = strcasecmp (app_name_a, app_name_b);
	} else if (is_category_a && is_special_b) {
		result = A_AFTER_B;
	} else if (is_category_a && is_separator_b) {
		result = A_AFTER_B;
	// then applications and distribution packages...
	} else if (app_name_a && app_name_b) {
		result = strcasecmp (app_name_a, app_name_b);
	} else if (app_name_a && !app_name_b) {
		result = A_BEFORE_B;
	} else if (!app_name_a && app_name_b) {
		result = A_AFTER_B;
	} else {
		result = strcasecmp (package_id_a, package_id_b);
	}

	g_free (app_name_a);
	g_free (app_name_b);
	g_free (package_id_a);
	g_free (package_id_b);

	return result;
}

gboolean
gpk_packages_list_row_separator_func (GtkTreeModel *model,
                                      GtkTreeIter  *iter,
                                      gpointer      user_data)
{
	gboolean is_separator = FALSE;
	gtk_tree_model_get (model, iter, PACKAGES_COLUMN_IS_SEPARATOR, &is_separator, -1);
	return is_separator;
}

GtkListStore *
gpk_packages_list_store_new (void)
{
	GtkListStore *store = gtk_list_store_new (PACKAGES_COLUMN_LAST,
	                                          GDK_TYPE_PIXBUF,
	                                          G_TYPE_STRING,
	                                          G_TYPE_STRING,
	                                          G_TYPE_STRING,
	                                          G_TYPE_STRING,
	                                          G_TYPE_BOOLEAN,
	                                          G_TYPE_BOOLEAN,
	                                          G_TYPE_BOOLEAN);

	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (store),
	                                 PACKAGES_COLUMN_ID,
	                                 gpk_packages_list_column_sort_func,
	                                 NULL, NULL);

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
	                                      PACKAGES_COLUMN_ID,
	                                      GTK_SORT_ASCENDING);

	return store;
}
