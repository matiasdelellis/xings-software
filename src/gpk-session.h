/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008-2009 Richard Hughes <richard@hughsie.com>
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
	void		(* idle_changed)		(GpkSession	*session,
							 gboolean	 is_idle);
	void		(* inhibited_changed)		(GpkSession	*session,
							 gboolean	 is_inhibited);
	/* just exit */
	void		(* stop)			(GpkSession	*session);
	/* reply with EndSessionResponse */
	void		(* query_end_session)		(GpkSession	*session,
							 guint		 flags);
	/* reply with EndSessionResponse */
	void		(* end_session)			(GpkSession	*session,
							 guint		 flags);
	void		(* cancel_end_session)		(GpkSession	*session);
};

GType		 gpk_session_get_type			(void);
GpkSession	*gpk_session_new			(void);

gboolean	 gpk_session_logout			(GpkSession	*session);

G_END_DECLS

#endif	/* __GPK_SESSION_H */
