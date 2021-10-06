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

#ifndef __GPK_SESSION_H
#define __GPK_SESSION_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GPK_TYPE_SESSION (gpk_session_get_type())

G_DECLARE_DERIVABLE_TYPE (GpkSession, gpk_session, GPK, SESSION, GObject)

struct _GpkSessionClass
{
	GObjectClass	parent_class;
};

GpkSession	*gpk_session_new			(void);

gboolean	 gpk_session_logout			(GpkSession	*session,
                                                         GError		**error);
gboolean	 gpk_session_reboot			(GpkSession	*session,
                                                         GError		**error);
gboolean	 gpk_session_can_reboot			(GpkSession	*session,
                                                         gboolean	*can_reboot,
                                                         GError		**error);

GDBusProxy *
gpk_session_get_proxy_if_service_present (GDBusConnection *connection,
                                          GDBusProxyFlags  flags,
                                          const char      *bus_name,
                                          const char      *object_path,
                                          const char      *interface,
                                          GError         **error);

G_END_DECLS

#endif	/* __GPK_SESSION_H */
