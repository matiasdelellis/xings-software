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

#ifndef __GPK_FLATPAK_INSTALLER
#define __GPK_FLATPAK_INSTALLER

#include <glib-object.h>

G_BEGIN_DECLS

#define GPK_TYPE_FLATPAK_INSTALLER (gpk_flatpak_installer_get_type())

G_DECLARE_DERIVABLE_TYPE (GpkFlatpakInstaller, gpk_flatpak_installer, GPK, FLATPAK_INSTALLER, GObject)

struct _GpkFlatpakInstallerClass
{
	GObjectClass	parent_class;
};

gboolean
gpk_flatpak_installer_launch_ready (GpkFlatpakInstaller *installer, GError **error);

guint64
gpk_flatpak_installer_get_download_size (GpkFlatpakInstaller *installer);

gchar *
gpk_flatpak_installer_get_name (GpkFlatpakInstaller *installer);

gboolean
gpk_flatpak_installer_perform_finish (GpkFlatpakInstaller  *installer,
                                      GAsyncResult         *result,
                                      GError              **error);

gboolean
gpk_flatpak_installer_perform_async (GpkFlatpakInstaller  *installer,
                                     GAsyncReadyCallback   ready_callback,
                                     gpointer              callback_data,
                                     GCancellable         *cancellable,
                                     GError              **error);

gboolean
gpk_flatpak_installer_prepare_finish (GpkFlatpakInstaller  *installer,
                                      GAsyncResult         *result,
                                      GError              **error);

gboolean
gpk_flatpak_installer_prepare_async (GpkFlatpakInstaller  *installer,
                                      gchar                *file,
                                      GAsyncReadyCallback   ready_callback,
                                      gpointer              callback_data,
                                      GCancellable         *cancellable,
                                      GError              **error);

GpkFlatpakInstaller *
gpk_flatpak_installer_new (void);

G_END_DECLS

#endif	/* __GPK_FLATPAK_INSTALLER */
