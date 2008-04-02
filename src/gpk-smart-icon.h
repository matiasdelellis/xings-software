/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 Richard Hughes <richard@hughsie.com>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __PK_SMART_ICON_H
#define __PK_SMART_ICON_H

#include <glib-object.h>

G_BEGIN_DECLS

#define PK_TYPE_SMART_ICON		(gpk_smart_icon_get_type ())
#define PK_SMART_ICON(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), PK_TYPE_SMART_ICON, GpkSmartIcon))
#define PK_SMART_ICON_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), PK_TYPE_SMART_ICON, GpkSmartIconClass))
#define PK_IS_SMART_ICON(o)	 	(G_TYPE_CHECK_INSTANCE_TYPE ((o), PK_TYPE_SMART_ICON))
#define PK_IS_SMART_ICON_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), PK_TYPE_SMART_ICON))
#define PK_SMART_ICON_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), PK_TYPE_SMART_ICON, GpkSmartIconClass))
#define PK_SMART_ICON_ERROR		(gpk_smart_icon_error_quark ())
#define PK_SMART_ICON_TYPE_ERROR	(gpk_smart_icon_error_get_type ())

typedef struct GpkSmartIconPrivate GpkSmartIconPrivate;

typedef struct
{
	 GObject		 parent;
	 GpkSmartIconPrivate	*priv;
} GpkSmartIcon;

typedef struct
{
	GObjectClass	parent_class;
} GpkSmartIconClass;

typedef enum
{
	GPK_NOTIFY_URGENCY_LOW,
	GPK_NOTIFY_URGENCY_NORMAL,
	GPK_NOTIFY_URGENCY_CRITICAL
} GpkNotifyUrgency;

typedef enum
{
	GPK_NOTIFY_TIMEOUT_SHORT,
	GPK_NOTIFY_TIMEOUT_LONG,
	GPK_NOTIFY_TIMEOUT_NEVER
} GpkNotifyTimeout;

typedef enum
{
	GPK_NOTIFY_BUTTON_DO_NOT_SHOW_AGAIN,
	GPK_NOTIFY_BUTTON_DO_NOT_WARN_AGAIN,
	GPK_NOTIFY_BUTTON_CANCEL_UPDATE,
	GPK_NOTIFY_BUTTON_UPDATE_COMPUTER,
	GPK_NOTIFY_BUTTON_RESTART_COMPUTER,
	GPK_NOTIFY_BUTTON_UNKNOWN
} GpkNotifyButton;

GType		 gpk_smart_icon_get_type		(void) G_GNUC_CONST;
GpkSmartIcon	*gpk_smart_icon_new			(void);
GtkStatusIcon	*gpk_smart_icon_get_status_icon		(GpkSmartIcon	*sicon);
gboolean	 gpk_smart_icon_sync			(GpkSmartIcon	*sicon);
gboolean	 gpk_smart_icon_set_icon_name		(GpkSmartIcon	*sicon,
							 const gchar	*icon_name);
gboolean	 gpk_smart_icon_set_tooltip		(GpkSmartIcon	*sicon,
							 const gchar	*tooltip);
gboolean	 gpk_smart_icon_notify_new		(GpkSmartIcon	*sicon,
							 const gchar	*title,
							 const gchar	*message,
							 const gchar	*icon,
							 GpkNotifyUrgency urgency,
							 GpkNotifyTimeout timeout);
gboolean	 gpk_smart_icon_notify_button		(GpkSmartIcon	*sicon,
							 GpkNotifyButton	 button,
							 const gchar	*data);
gboolean	 gpk_smart_icon_notify_show		(GpkSmartIcon	*sicon);
gboolean	 gpk_smart_icon_notify_close		(GpkSmartIcon	*sicon);


G_END_DECLS

#endif /* __PK_SMART_ICON_H */
