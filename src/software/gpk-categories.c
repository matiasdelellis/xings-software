/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2022 Matias De lellis <matias@delellis.com.ar>
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

#include <string.h>
#include <glib.h>

#include "gpk-categories.h"

static void     gpk_categories_finalize	(GObject	  *object);

struct _GpkCategories
{
	GObject			 parent;

	GHashTable		*categories;
};

G_DEFINE_TYPE (GpkCategories, gpk_categories, G_TYPE_OBJECT)

gboolean
gpk_categories_load (GpkCategories *categories, GError **error)
{
	GDir *categories_folder = NULL;
	GpkCategory *category = NULL;
	const gchar *base_file = NULL;
	gchar *file = NULL;

	// Load categories
	categories_folder = g_dir_open(PKGDATADIR"/categories", 0, error);
	if (!categories_folder)
		return FALSE;

	base_file = g_dir_read_name(categories_folder);
	while (base_file) {
		category = gpk_category_new ();

		file = g_strconcat(PKGDATADIR"/categories", G_DIR_SEPARATOR_S, base_file, NULL);
		gpk_category_load (category, file, NULL);
		g_free (file);

		g_hash_table_insert (categories->categories,
		                     gpk_category_get_id (category),
		                     category);

		base_file = g_dir_read_name(categories_folder);
	}

	g_debug ("System categories: %u", g_hash_table_size(categories->categories));

	g_dir_close(categories_folder);

	return TRUE;
}

GpkCategory *
gpk_categories_get_by_id (GpkCategories *categories, const gchar *id)
{
	return g_hash_table_lookup (categories->categories, id);
}

GPtrArray *
gpk_categories_get_principals (GpkCategories *categories)
{
	GHashTableIter ht_iter;
	gpointer ht_value;

	GPtrArray *results = g_ptr_array_new_with_free_func (g_object_unref);

	g_hash_table_iter_init (&ht_iter, categories->categories);
	while (g_hash_table_iter_next (&ht_iter, NULL, &ht_value))
		g_ptr_array_add (results, g_object_ref (ht_value));

	return results;
}

static void
gpk_categories_finalize (GObject *object)
{
	GpkCategories *categories;

	g_return_if_fail (GPK_IS_CATEGORIES (object));

	categories = GPK_CATEGORIES (object);

	g_hash_table_unref (categories->categories);

	G_OBJECT_CLASS (gpk_categories_parent_class)->finalize (object);
}

static void
gpk_categories_init (GpkCategories *categories)
{
	categories->categories = g_hash_table_new_full (g_str_hash, g_str_equal, (GDestroyNotify) g_free, (GDestroyNotify) g_object_unref);
}

static void
gpk_categories_class_init (GpkCategoriesClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gpk_categories_finalize;
}

GpkCategories *
gpk_categories_new (void)
{
	GpkCategories *categories;
	categories = g_object_new (GPK_TYPE_CATEGORIES, NULL);
	return GPK_CATEGORIES (categories);
}
