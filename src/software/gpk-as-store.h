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

#ifndef __GPK_AS_STORE_H
#define __GPK_AS_STORE_H

#include <glib-object.h>

#include <appstream.h>

G_BEGIN_DECLS

#define GPK_TYPE_AS_STORE (gpk_as_store_get_type())
G_DECLARE_FINAL_TYPE (GpkAsStore, gpk_as_store, GPK, AS_STORE, GObject)

GpkAsStore*
gpk_as_store_new (void);

gboolean
gpk_as_store_load (GpkAsStore *store, GCancellable *cancellable, GError **error);

gchar *
gpk_as_component_get_desktop_id (AsComponent *component);

AsComponent *
gpk_as_store_get_component_by_pkgname (GpkAsStore *store, const gchar *pkgname);

gchar **
gpk_as_store_search_pkgnames_by_categories (GpkAsStore *store, gchar **includes, gchar **excludes);

gchar **
gpk_as_store_search_pkgnames (GpkAsStore *store, const gchar *search);


G_END_DECLS

#endif /* __GPK_AS_STORE_H */
