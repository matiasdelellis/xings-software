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

#ifndef __GPK_FLATPAK_REF
#define __GPK_FLATPAK_REF

#include <glib-object.h>

G_BEGIN_DECLS

#define GPK_TYPE_FLATPAK_REF (gpk_flatpak_ref_get_type())

G_DECLARE_DERIVABLE_TYPE (GpkFlatpakRef, gpk_flatpak_ref, GPK, FLATPAK_REF, GObject)

struct _GpkFlatpakRefClass
{
	GObjectClass	parent_class;
};

gchar *
gpk_flatpak_ref_get_name (GpkFlatpakRef *flatpakref);

gchar *
gpk_flatpak_ref_get_branch (GpkFlatpakRef *flatpakref);

gchar *
gpk_flatpak_ref_get_title (GpkFlatpakRef *flatpakref);

GBytes *
gpk_flatpak_ref_get_raw_data (GpkFlatpakRef *flatpakref);

gboolean
gpk_flatpak_ref_load_from_file (GpkFlatpakRef *flatpakref, const gchar *file, GError **error);


GpkFlatpakRef *
gpk_flatpak_ref_new (void);

G_END_DECLS

#endif	/* __GPK_FLATPAK_REF */
