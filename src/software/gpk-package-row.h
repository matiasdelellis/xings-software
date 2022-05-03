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

#ifndef GPK_PACKAGE_ROW_H
#define GPK_PACKAGE_ROW_H

G_BEGIN_DECLS

#define GPK_TYPE_PACKAGE_ROW (gpk_package_row_get_type ())
G_DECLARE_FINAL_TYPE (GpkPackageRow, gpk_package_row, GPK, PACKAGE_ROW, GtkListBoxRow)

gint
gpk_package_row_sort_func (GtkListBoxRow *a,
                           GtkListBoxRow *b,
                           gpointer       user_data);

void
gpk_package_row_set_component (GpkPackageRow *row,
                               AsComponent   *component);

GtkWidget *
gpk_package_row_new (PkPackage   *package);

G_END_DECLS

#endif
