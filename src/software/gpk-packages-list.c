/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2022 Matias De lellis <mati86dl@gmail.com>
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

static gint
gpk_packages_list_column_sort_func (GtkTreeModel *model,
                                    GtkTreeIter  *a,
                                    GtkTreeIter  *b,
                                    gpointer      user_data)
{
	gchar *app_name_a, *app_name_b;
	gchar *package_id_a, *package_id_b;
	gint result = 0;

	gtk_tree_model_get (model, a,
	                    PACKAGES_COLUMN_APP_NAME, &app_name_a,
	                    PACKAGES_COLUMN_ID, &package_id_a,
	                    -1);
	gtk_tree_model_get (model, b,
	                    PACKAGES_COLUMN_APP_NAME, &app_name_b,
	                    PACKAGES_COLUMN_ID, &package_id_b,
	                    -1);

	if (app_name_a && app_name_b) {
		result = strcasecmp (app_name_a, app_name_b);
	} else if (app_name_a && !app_name_b) {
		result = -1;
	} else if (!app_name_a && app_name_b) {
		result = 1;
	} else {
		result = strcasecmp (package_id_a, package_id_b);
	}

	g_free (app_name_a);
	g_free (app_name_b);
	g_free (package_id_a);
	g_free (package_id_b);

	return result;
}

GtkListStore *
gpk_packages_list_store_new (void)
{
	GtkListStore *store = gtk_list_store_new (PACKAGES_COLUMN_LAST,
	                                          G_TYPE_STRING,
	                                          G_TYPE_UINT64,
	                                          G_TYPE_STRING,
	                                          G_TYPE_STRING,
	                                          G_TYPE_STRING,
	                                          G_TYPE_STRING);

	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (store),
	                                 PACKAGES_COLUMN_ID,
	                                 gpk_packages_list_column_sort_func,
	                                 NULL, NULL);

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
	                                      PACKAGES_COLUMN_ID,
	                                      GTK_SORT_ASCENDING);

	return store;
}
