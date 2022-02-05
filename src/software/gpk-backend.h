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

#ifndef __GPK_BACKEND_H
#define __GPK_BACKEND_H

#include <glib-object.h>
#include <appstream.h>

#include "gpk-categories.h"

G_BEGIN_DECLS

#define GPK_TYPE_BACKEND (gpk_backend_get_type())
G_DECLARE_FINAL_TYPE (GpkBackend, gpk_backend, GPK, BACKEND, GObject)

AsComponent *
gpk_backend_get_component_by_pkgname (GpkBackend *backend, const gchar *pkgname);

GpkCategory *
gpk_backend_get_category_by_id (GpkBackend *backend, const gchar *id);

GPtrArray *
gpk_backend_get_principals_categories (GpkBackend *backend);

gchar **
gpk_backend_search_pkgnames_with_component (GpkBackend *backend, const gchar *search);

gchar **
gpk_backend_search_pkgnames_by_categories (GpkBackend *backend, gchar **categories);

const gchar *
gpk_backend_get_full_repo_name (GpkBackend *backend, const gchar *repo_id);

PkTask *
gpk_backend_get_task (GpkBackend *backend);

gboolean
gpk_backend_open_finish (GpkBackend    *backend,
                         GAsyncResult  *result,
                         GError       **error);

void
gpk_backend_open (GpkBackend          *backend,
                  GCancellable        *cancellable,
                  GAsyncReadyCallback  ready_callback,
                  gpointer             user_data);

GpkBackend *
gpk_backend_new (void);

G_END_DECLS

#endif /* __GPK_BACKEND_H */
