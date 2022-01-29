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
#include <glib/gi18n.h>

#include "gpk-groups-list.h"

void
gpk_groups_list_add_pending (GtkTreeStore *store)
{
	gboolean ret;
	gchar *id = NULL;
	GtkTreeIter iter;

	ret = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter);
	if (!ret)
		goto out;

	gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
	                    GROUPS_COLUMN_ID, &id,
	                    -1);

	if (g_strcmp0 (id, "selected") == 0)
		goto out;

	gtk_tree_store_insert (store, &iter, NULL, 0);
	gtk_tree_store_set (store, &iter,
	                    /* TRANSLATORS: this is a menu group of packages in the queue */
	                    GROUPS_COLUMN_NAME, _("Pending Changes"),
	                    GROUPS_COLUMN_SUMMARY, NULL,
	                    GROUPS_COLUMN_ID, "selected",
	                    GROUPS_COLUMN_ICON, "edit-find",
	                    GROUPS_COLUMN_ACTIVE, TRUE,
	                    -1);

out:
	g_free (id);
}

void
gpk_groups_list_remove_pending (GtkTreeStore *store)
{
	gboolean ret;
	gchar *id = NULL;
	GtkTreeIter iter;

	ret = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter);
	if (!ret)
		goto out;

	gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
	                    GROUPS_COLUMN_ID, &id,
	                   -1);

	if (g_strcmp0 (id, "selected") != 0)
		goto out;

	gtk_tree_store_remove (store, &iter);

out:
	g_free (id);
}

void
gpk_groups_list_append (GtkTreeStore *store, PkGroupEnum group)
{
	GtkTreeIter iter;
	const gchar *icon_name;
	const gchar *text;

	text = gpk_group_enum_to_localised_text (group);
	icon_name = gpk_group_enum_to_icon_name (group);

	gtk_tree_store_append (store, &iter, NULL);
	gtk_tree_store_set (store, &iter,
	                    GROUPS_COLUMN_NAME, text,
	                    GROUPS_COLUMN_SUMMARY, NULL,
	                    GROUPS_COLUMN_ID, pk_group_enum_to_string (group),
	                    GROUPS_COLUMN_ICON, icon_name,
	                    GROUPS_COLUMN_ACTIVE, TRUE,
	                    -1);
}

void
gpk_groups_list_append_enumerated (GtkTreeStore *store,
                                   GtkTreeView  *view,
                                   PkBitfield    groups,
                                   PkBitfield    roles)
{
	guint i;

	/* set to no indent */
	gtk_tree_view_set_show_expanders (view, FALSE);
	gtk_tree_view_set_level_indentation  (view, 0);

	/* create group tree view if we can search by group */
	if (pk_bitfield_contain (roles, PK_ROLE_ENUM_SEARCH_GROUP)) {
		/* add all the groups supported (except collections, which we handled above */
		for (i = 0; i < PK_GROUP_ENUM_LAST; i++) {
			if (pk_bitfield_contain (groups, i) &&
			    i != PK_GROUP_ENUM_COLLECTIONS && i != PK_GROUP_ENUM_NEWEST)
				gpk_groups_list_append (store, i);
		}
	}
}


GtkTreeStore *
gpk_groups_list_store_new (void)
{
	GtkTreeStore *store = gtk_tree_store_new (GROUPS_COLUMN_LAST,
	                                          G_TYPE_STRING,
	                                          G_TYPE_STRING,
	                                          G_TYPE_STRING,
	                                          G_TYPE_STRING,
	                                          G_TYPE_BOOLEAN);

	return store;
}
