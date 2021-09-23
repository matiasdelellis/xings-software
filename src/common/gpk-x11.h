/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Richard Hughes <richard@hughsie.com>
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

#ifndef __GPK_X11_H
#define __GPK_X11_H

#include <glib-object.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS

#define GPK_TYPE_X11 (gpk_x11_get_type())
G_DECLARE_FINAL_TYPE (GpkX11, gpk_x11, GPK, X11, GObject)

GpkX11		*gpk_x11_new				(void);
gboolean	 gpk_x11_set_xid			(GpkX11		*x11,
							 guint32	 xid);
gboolean	 gpk_x11_set_window			(GpkX11		*x11,
							 GdkWindow	*window);
guint32		 gpk_x11_get_user_time			(GpkX11		*x11);
gchar		*gpk_x11_get_title			(GpkX11		*x11);

G_END_DECLS

#endif /* __GPK_X11_H */
