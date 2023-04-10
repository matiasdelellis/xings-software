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

#include <glib.h>
#include <gio/gio.h>

#include "gpk-flatpak-ref.h"


#define REF_GROUP  "Flatpak Ref"
#define KEY_NAME   "Name"
#define KEY_TITLE  "Title"
#define KEY_BRANCH "Branch"


static void     gpk_flatpak_ref_finalize   (GObject		*object);

typedef struct
{
	GKeyFile *keyfile;
} GpkFlatpakRefPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GpkFlatpakRef, gpk_flatpak_ref, G_TYPE_OBJECT)


gchar *
gpk_flatpak_ref_get_name (GpkFlatpakRef *flatpakref)
{
	GpkFlatpakRefPrivate *priv;

	priv = gpk_flatpak_ref_get_instance_private (GPK_FLATPAK_REF (flatpakref));

	return g_key_file_get_string (priv->keyfile, REF_GROUP, KEY_NAME, NULL);
}

gchar *
gpk_flatpak_ref_get_title (GpkFlatpakRef *flatpakref)
{
	GpkFlatpakRefPrivate *priv;

	priv = gpk_flatpak_ref_get_instance_private (GPK_FLATPAK_REF (flatpakref));

	return g_key_file_get_string (priv->keyfile, REF_GROUP, KEY_TITLE, NULL);
}

gchar *
gpk_flatpak_ref_get_branch (GpkFlatpakRef *flatpakref)
{
	GpkFlatpakRefPrivate *priv;

	priv = gpk_flatpak_ref_get_instance_private (GPK_FLATPAK_REF (flatpakref));

	return g_key_file_get_string (priv->keyfile, REF_GROUP, KEY_BRANCH, NULL);
}

GBytes *
gpk_flatpak_ref_get_raw_data (GpkFlatpakRef *flatpakref)
{
	GpkFlatpakRefPrivate *priv;
	GBytes *raw_data_b = NULL;
	gchar *raw_data_c = NULL;
	gsize lenght = 0;

	priv = gpk_flatpak_ref_get_instance_private (GPK_FLATPAK_REF (flatpakref));

	raw_data_c = g_key_file_to_data (priv->keyfile, &lenght, NULL);
	raw_data_b = g_bytes_new (raw_data_c, lenght);
	g_free (raw_data_c);

	return raw_data_b;
}

gboolean
gpk_flatpak_ref_load_from_file (GpkFlatpakRef *flatpakref, const gchar *file, GError **error)
{
	GpkFlatpakRefPrivate *priv;

	priv = gpk_flatpak_ref_get_instance_private (GPK_FLATPAK_REF (flatpakref));

	return g_key_file_load_from_file (priv->keyfile,
	                                  file,
	                                  G_KEY_FILE_NONE,
	                                  error);
}

static void
gpk_flatpak_ref_class_init (GpkFlatpakRefClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gpk_flatpak_ref_finalize;
}

static void
gpk_flatpak_ref_init (GpkFlatpakRef *flatpakref)
{
	GpkFlatpakRefPrivate *priv;

	priv = gpk_flatpak_ref_get_instance_private (GPK_FLATPAK_REF (flatpakref));

	priv->keyfile = g_key_file_new();
}

static void
gpk_flatpak_ref_finalize (GObject *object)
{
	GpkFlatpakRefPrivate *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GPK_IS_FLATPAK_REF (object));

	priv = gpk_flatpak_ref_get_instance_private (GPK_FLATPAK_REF (object));

	if (priv->keyfile) {
		g_key_file_free (priv->keyfile);
		priv->keyfile = NULL;
	}

	G_OBJECT_CLASS (gpk_flatpak_ref_parent_class)->finalize (object);
}

GpkFlatpakRef *
gpk_flatpak_ref_new (void)
{
	GpkFlatpakRef *flatpakref = g_object_new (GPK_TYPE_FLATPAK_REF, NULL);
	return GPK_FLATPAK_REF (flatpakref);
}
