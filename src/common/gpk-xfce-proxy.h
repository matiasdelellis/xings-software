/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2021 Matias De lellis <mati86dl@gmail.com>
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

#ifndef __GPK_XFCE_PROXY_H
#define __GPK_XFCE_PROXY_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GPK_TYPE_XFCE_PROXY (gpk_xfce_proxy_get_type())

G_DECLARE_DERIVABLE_TYPE (GpkXfceProxy, gpk_xfce_proxy, GPK, XFCE_PROXY, GObject)

struct _GpkXfceProxyClass
{
	GObjectClass	parent_class;
};

gboolean	 gpk_xfce_proxy_logout			(GpkXfceProxy	 *proxy,
							 GError		**error);

gboolean	 gpk_xfce_proxy_reboot			(GpkXfceProxy	*proxy,
							 GError		**error);

gboolean	 gpk_xfce_proxy_is_conected 		(GpkXfceProxy	*proxy);

GpkXfceProxy	*gpk_xfce_proxy_new			(void);

G_END_DECLS

#endif	/* __GPK_XFCE_PROXY_H */
