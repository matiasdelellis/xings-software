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

#ifndef __GPK_CATEGORY_H
#define __GPK_CATEGORY_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GPK_TYPE_CATEGORY (gpk_category_get_type())
G_DECLARE_FINAL_TYPE (GpkCategory, gpk_category, GPK, CATEGORY, GObject)

gchar *
gpk_category_get_id (GpkCategory *category);

gchar *
gpk_category_get_name (GpkCategory *category);

gchar *
gpk_category_get_icon (GpkCategory *category);

gchar *
gpk_category_get_comment (GpkCategory *category);

gchar **
gpk_category_get_categories (GpkCategory *category);

gchar **
gpk_category_get_packages (GpkCategory *category);

gboolean
gpk_category_is_special (GpkCategory *category);

GpkCategory *
gpk_category_new (void);

gboolean
gpk_category_load (GpkCategory *category, const gchar *file, GError **error);

G_END_DECLS

#endif /* __GPK_CATEGORY_H */
