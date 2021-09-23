/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2009 Richard Hughes <richard@hughsie.com>
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

#ifndef __GPK_HELPER_RUN_H
#define __GPK_HELPER_RUN_H

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GPK_TYPE_HELPER_RUN (gpk_helper_run_get_type())
G_DECLARE_FINAL_TYPE (GpkHelperRun, gpk_helper_run, GPK, HELPER_RUN, GObject)

//GType		 gpk_helper_run_get_type	  	(void);
GpkHelperRun	*gpk_helper_run_new			(void);
gboolean	 gpk_helper_run_set_parent		(GpkHelperRun		*helper,
							 GtkWindow		*window);
gboolean	 gpk_helper_run_show			(GpkHelperRun		*helper,
							 gchar			**package_ids);

G_END_DECLS

#endif /* __GPK_HELPER_RUN_H */
