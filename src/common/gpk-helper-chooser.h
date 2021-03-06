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

#ifndef __GPK_HELPER_CHOOSER_H
#define __GPK_HELPER_CHOOSER_H

#include <glib-object.h>
#include <gtk/gtk.h>
#include <packagekit-glib2/packagekit.h>

G_BEGIN_DECLS

#define GPK_TYPE_HELPER_CHOOSER (gpk_helper_chooser_get_type())

G_DECLARE_DERIVABLE_TYPE (GpkHelperChooser, gpk_helper_chooser, GPK, HELPER_CHOOSER, GObject)

struct _GpkHelperChooserClass
{
	GObjectClass	parent_class;

	void		(* event)			(GpkHelperChooser	*helper,
							 GtkResponseType	 type,
							 const gchar		*package_id);
};

GpkHelperChooser	*gpk_helper_chooser_new		(void);
gboolean	 gpk_helper_chooser_set_parent		(GpkHelperChooser	*helper,
							 GtkWindow		*window);
gboolean	 gpk_helper_chooser_show		(GpkHelperChooser	*helper,
							 GPtrArray		*list);

G_END_DECLS

#endif /* __GPK_HELPER_CHOOSER_H */
