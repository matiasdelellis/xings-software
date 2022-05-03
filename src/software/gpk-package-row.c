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


#include "config.h"

#include <common/gpk-common.h>
#include <common/gpk-enum.h>
#include <gtk/gtk.h>

#include <appstream.h>

#include "gpk-package-row.h"

struct _GpkPackageRow
{
	GtkListBoxRow  parent;

	PkPackage     *package;
	AsComponent   *component;

	GtkWidget     *image;
	GtkWidget     *title;
	GtkWidget     *subtitle;
};

enum
{
	PROP_0,
	PROP_PACKAGE,
	PROP_COMPONENT,
	LAST_PROP
};

static GParamSpec *row_properties[LAST_PROP] = { NULL };

G_DEFINE_TYPE (GpkPackageRow, gpk_package_row, GTK_TYPE_LIST_BOX_ROW)

static GdkPixbuf *
gpk_get_pixbuf_from_component (AsComponent *component, gint size)
{
	GPtrArray *icons = NULL;
	AsIcon *icon = NULL;
	GdkPixbuf *pixbuf = NULL, *scaled_pixbuf = NULL;
	const gchar *filename = NULL;
	guint i = 0;

	icons = as_component_get_icons(component);
	for (i = 0; i < icons->len; i++) {
		icon = AS_ICON (g_ptr_array_index (icons, i));
		if (as_icon_get_kind (icon) != AS_ICON_KIND_STOCK) {
			filename = as_icon_get_filename (icon);
			pixbuf = gdk_pixbuf_new_from_file (filename, NULL);
			if (pixbuf) break;
		}
	}

	if (pixbuf) {
		scaled_pixbuf = gdk_pixbuf_scale_simple (pixbuf,
		                                         size,
		                                         size,
		                                         GDK_INTERP_BILINEAR);
		g_object_unref (pixbuf);
		pixbuf = scaled_pixbuf;
	}

	return pixbuf;
}

static GdkPixbuf *
gpk_get_pixbuf_from_icon_name (const gchar *icon_name, gint size)
{
	return gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
	                                 icon_name,
	                                 size,
	                                 GTK_ICON_LOOKUP_FORCE_SIZE,
	                                 NULL);
}

static GdkPixbuf *
gpk_package_row_get_pixbuf (GpkPackageRow *self)
{
	GdkPixbuf *pixbuf = NULL;
	PkInfoEnum info;

	if (self->component) {
		pixbuf = gpk_get_pixbuf_from_component (self->component, 32);
	}

	if (!pixbuf) {
		g_object_get (self->package, "info", &info, NULL);
		pixbuf = gpk_get_pixbuf_from_icon_name (gpk_info_enum_to_icon_name (info), 32);
	}

	return pixbuf;
}

static gchar *
gpk_package_row_get_title (GpkPackageRow *self)
{
	gchar *package_id = NULL, *title = NULL;

	if (self->component) {
		title = g_strdup (as_component_get_name (self->component));
	}

	if (!title) {
		g_object_get (self->package, "package-id", &package_id, NULL);
		title = gpk_package_id_get_name (package_id);
		g_free (package_id);
	}

	return title;
}

static gchar *
gpk_package_row_get_subtitle (GpkPackageRow *self)
{
	gchar *subtitle = NULL;

	if (self->component) {
		subtitle = g_strdup (as_component_get_summary (self->component));
	}

	if (!subtitle) {
		g_object_get (self->package, "summary", &subtitle, NULL);
	}

	return subtitle;
}

static void
gpk_package_row_constructed (GObject *object)
{
	GpkPackageRow *self;
	GtkWidget *widget, *hbox, *vbox;
	GdkPixbuf *pixbuf;

	self = GPK_PACKAGE_ROW(object);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

	widget = gtk_image_new();
	pixbuf = gpk_package_row_get_pixbuf (self);
	if (pixbuf)
		gtk_image_set_from_pixbuf (GTK_IMAGE(widget), pixbuf);
	gtk_box_pack_start (GTK_BOX(hbox), widget, FALSE, FALSE, 4);
	self->image = widget;

	widget = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL(widget), gpk_package_row_get_title (self));
	gtk_widget_set_halign (widget, GTK_ALIGN_START);
	gtk_box_pack_start (GTK_BOX(vbox), widget, FALSE, FALSE, 0);
	self->title = widget;

	widget = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL(widget), gpk_package_row_get_subtitle (self));
	gtk_widget_set_halign (widget, GTK_ALIGN_START);
	gtk_box_pack_end (GTK_BOX(vbox), widget, FALSE, FALSE, 0);
	self->subtitle = widget;

	gtk_box_pack_end (GTK_BOX(hbox), vbox, TRUE, TRUE, 2);

	gtk_container_add (GTK_CONTAINER(object), hbox);

	G_OBJECT_CLASS (gpk_package_row_parent_class)->constructed (object);
}

static void
gpk_package_row_dispose (GObject *object)
{
	GpkPackageRow *self;

	self = GPK_PACKAGE_ROW (object);

	g_clear_object (&self->component);
	g_clear_object (&self->package);

	G_OBJECT_CLASS (gpk_package_row_parent_class)->dispose (object);
}

static void
gpk_package_row_finalize (GObject *object)
{
	G_OBJECT_CLASS (gpk_package_row_parent_class)->finalize (object);
}

static void
gpk_package_row_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
	GpkPackageRow *self;

	self = GPK_PACKAGE_ROW (object);

	switch (property_id) {
		case PROP_PACKAGE:
			self->package = g_value_dup_object (value);
			break;
		case PROP_COMPONENT:
			gpk_package_row_set_component (self, g_value_dup_object (value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gpk_package_row_class_init (GpkPackageRowClass *self_class)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (self_class);

	object_class->constructed = gpk_package_row_constructed;
	object_class->dispose = gpk_package_row_dispose;
	object_class->finalize = gpk_package_row_finalize;
	object_class->set_property = gpk_package_row_set_property;

	row_properties[PROP_PACKAGE] =
		g_param_spec_object ("package",
		                     "package",
		                     "package",
		                     PK_TYPE_PACKAGE,
		                     G_PARAM_WRITABLE |
		                     G_PARAM_CONSTRUCT_ONLY |
		                     G_PARAM_STATIC_STRINGS);

	row_properties[PROP_COMPONENT] =
		g_param_spec_object ("component",
		                     "component",
		                     "component",
		                     AS_TYPE_COMPONENT,
		                     G_PARAM_WRITABLE |
		                     G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties (object_class, LAST_PROP, row_properties);
}

static void
gpk_package_row_init (GpkPackageRow *self)
{
}

void
gpk_package_row_set_component (GpkPackageRow *row,
                               AsComponent   *component)
{
	row->component = component;
	gtk_image_set_from_pixbuf (GTK_IMAGE(row->image), gpk_package_row_get_pixbuf (row));
	gtk_label_set_markup (GTK_LABEL(row->title), gpk_package_row_get_title (row));
	gtk_label_set_markup (GTK_LABEL(row->subtitle), gpk_package_row_get_subtitle (row));
}

GtkWidget *
gpk_package_row_new (PkPackage   *package)
{
	return g_object_new (GPK_TYPE_PACKAGE_ROW,
	                     "package", package,
	                     NULL);
}
