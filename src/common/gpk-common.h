/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007-2008 Richard Hughes <richard@hughsie.com>
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

#ifndef __GPK_COMMON_H
#define __GPK_COMMON_H

#include <glib-object.h>
#include <gtk/gtk.h>
#include <packagekit-glib2/packagekit.h>

#include "gpk-enum.h"

G_BEGIN_DECLS

#define GPK_SETTINGS_SCHEMA				"org.xings.software"
#define GPK_SETTINGS_DBUS_DEFAULT_INTERACTION		"dbus-default-interaction"
#define GPK_SETTINGS_DBUS_ENFORCED_INTERACTION		"dbus-enforced-interaction"
#define GPK_SETTINGS_ENABLE_AUTOREMOVE			"enable-autoremove"
#define GPK_SETTINGS_ENABLE_CODEC_HELPER		"enable-codec-helper"
#define GPK_SETTINGS_ENABLE_FONT_HELPER			"enable-font-helper"
#define GPK_SETTINGS_ENABLE_MIME_TYPE_HELPER		"enable-mime-type-helper"
#define GPK_SETTINGS_IGNORED_DBUS_REQUESTS		"ignored-dbus-requests"
#define GPK_SETTINGS_REPO_SHOW_DETAILS			"repo-show-details"
#define GPK_SETTINGS_SCROLL_ACTIVE			"scroll-active"
#define GPK_SETTINGS_SEARCH_MODE			"search-mode"
#define GPK_SETTINGS_SHOW_DEPENDS			"show-depends"
#define GPK_SETTINGS_UPDATE_ON_MOBILE			"update-on-mobile"
#define GPK_SETTINGS_AUTO_DOWNLOAD_UPDATES		"auto-download-updates"
#define GPK_SETTINGS_FREQUENCY_GET_UPDATES		"frequency-get-updates"
#define GPK_SETTINGS_FREQUENCY_REFRESH_CACHE		"frequency-refresh-cache"
#define GPK_SETTINGS_FREQUENCY_UPDATES_NOTIFICATION	"frequency-updates-notification"
#define GPK_SETTINGS_LAST_UPDATES_NOTIFICATION		"last-updates-notification"

#define GPK_ICON_SOFTWARE_INSTALLER			"system-software-install"
#define GPK_ICON_SOFTWARE_UPDATE			"system-software-update"
#define GPK_ICON_SOFTWARE_PREFERENCES			"xings-software-preferences"
#define GPK_ICON_SOFTWARE_HISTORY			"xings-software-history"

#define GPK_ICON_UPDATES_NORMAL				"software-update-available-symbolic"
#define GPK_ICON_UPDATES_URGENT				"software-update-urgent-symbolic"

/* any status that is slower than this will not be shown in the UI */
#define GPK_UI_STATUS_SHOW_DELAY		750 /* ms */

gchar		*gpk_package_id_get_name		(const gchar    *package_id);

gchar		*gpk_common_format_twoline		(const gchar   *header,
							 const gchar   *subtitle);
gchar		*gpk_package_id_format_twoline		(const gchar 	*package_id,
							 const gchar	*summary);
gchar		*gpk_package_id_format_oneline		(const gchar 	*package_id,
							 const gchar	*summary);
gchar		*gpk_package_id_format_pretty		(const gchar	*package_id);
gchar		*gpk_time_to_localised_string		(guint		 time_secs);
gchar		*gpk_time_to_imprecise_string		(guint		 time_secs);
gchar		*gpk_time_ago_to_localised_string	(guint		 seconds_ago);

gboolean	 gpk_check_privileged_user		(const gchar	*application_name,
							 gboolean	 show_ui);
gchar		*gpk_strv_join_locale			(gchar		**array);
gboolean	 gpk_window_set_size_request		(GtkWindow	*window,
							 guint		 width,
							 guint		 height);
gboolean	 gpk_window_set_parent_xid		(GtkWindow	*window,
							 guint32	 xid);
GPtrArray	*pk_strv_to_ptr_array			(gchar		**array)
							 G_GNUC_WARN_UNUSED_RESULT;
gchar		**pk_package_array_to_strv		(GPtrArray	*array);

G_END_DECLS

#endif	/* __GPK_COMMON_H */
