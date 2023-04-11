/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2022-2023 Matias De lellis <matias@delellis.com.ar>
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

#include "gpk-category.h"

static void     gpk_category_finalize	(GObject	  *object);

struct _GpkCategory
{
	GObject			 parent;

	GKeyFile		*key_file;
};

G_DEFINE_TYPE (GpkCategory, gpk_category, G_TYPE_OBJECT)

gchar *
gpk_category_get_id (GpkCategory *category)
{
	return g_key_file_get_string (category->key_file, "Desktop Entry", "Name", NULL);
}

gchar *
gpk_category_get_name (GpkCategory *category)
{
	return g_key_file_get_locale_string (category->key_file, "Desktop Entry", "Name", NULL, NULL);
}

gchar *
gpk_category_get_icon (GpkCategory *category)
{
	return g_key_file_get_locale_string (category->key_file, "Desktop Entry", "Icon", NULL, NULL);
}

gchar *
gpk_category_get_comment (GpkCategory *category)
{
	return g_key_file_get_locale_string (category->key_file, "Desktop Entry", "Comment", NULL, NULL);
}

gchar **
gpk_category_get_categories (GpkCategory *category)
{
	return g_key_file_get_string_list (category->key_file, "Desktop Entry", "Include", NULL, NULL);
}

gchar **
gpk_category_get_packages (GpkCategory *category)
{
	return g_key_file_get_string_list (category->key_file, "Desktop Entry", "Packages", NULL, NULL);
}

gboolean
gpk_category_is_special (GpkCategory *category)
{
	gboolean is_special = FALSE;
	gchar **packages = NULL;

	packages = gpk_category_get_packages (category);
	is_special = (packages != NULL);
	g_strfreev (packages);

	return is_special;
}

gboolean
gpk_category_load (GpkCategory *category, const gchar *file, GError **error)
{
	return g_key_file_load_from_file (category->key_file, file, G_KEY_FILE_NONE, error);
}

static void
gpk_category_finalize (GObject *object)
{
	GpkCategory *category;

	g_return_if_fail (GPK_IS_CATEGORY (object));

	category = GPK_CATEGORY (object);

	g_key_file_unref (category->key_file);

	G_OBJECT_CLASS (gpk_category_parent_class)->finalize (object);
}

static void
gpk_category_init (GpkCategory *category)
{
	category->key_file = g_key_file_new ();
}

static void
gpk_category_class_init (GpkCategoryClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gpk_category_finalize;
}

GpkCategory *
gpk_category_new (void)
{
	GpkCategory *category;
	category = g_object_new (GPK_TYPE_CATEGORY, NULL);
	return GPK_CATEGORY (category);
}
