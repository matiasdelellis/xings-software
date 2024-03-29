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

#include "config.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <dbus/dbus-glib.h>
#include <packagekit-glib2/packagekit.h>
#include <locale.h>

#include "gpk-enum.h"
#include "gpk-common.h"
#include "gpk-error.h"

/* if the dialog is going to cover more than this much of the screen, then maximize it at startup */
#define GPK_SMALL_FORM_FACTOR_SCREEN_PERCENT	75 /* % */

/* static, so local to process */
static gboolean small_form_factor_mode = FALSE;

gchar **
pk_package_array_to_strv (GPtrArray *array)
{
	PkPackage *item;
	gchar **results;
	guint i;

	results = g_new0 (gchar *, array->len+1);
	for (i=0; i<array->len; i++) {
		item = g_ptr_array_index (array, i);
		results[i] = g_strdup (pk_package_get_id (item));
	}
	return results;
}

/**
 * pk_strv_to_ptr_array:
 * @array: the gchar** array of strings
 *
 * Form a GPtrArray array of strings.
 * The data in the array is copied.
 *
 * Return value: the string array, or %NULL if invalid
 **/
GPtrArray *
pk_strv_to_ptr_array (gchar **array)
{
	guint i;
	guint length;
	GPtrArray *parray;

	g_return_val_if_fail (array != NULL, NULL);

	parray = g_ptr_array_new ();
	length = g_strv_length (array);
	for (i=0; i<length; i++)
		g_ptr_array_add (parray, g_strdup (array[i]));
	return parray;
}

/**
 * gpk_window_set_size_request:
 **/
gboolean
gpk_window_set_size_request (GtkWindow *window, guint width, guint height)
{
#ifdef PK_BUILD_SMALL_FORM_FACTOR
	GdkScreen *screen;
	guint screen_w;
	guint screen_h;
	guint percent_w;
	guint percent_h;

	/* check for tiny screen, like for instance a OLPC or EEE */
	screen = gdk_screen_get_default ();
	screen_w = gdk_screen_get_width (screen);
	screen_h = gdk_screen_get_height (screen);

	/* find percentage of screen area */
	percent_w = (width * 100) / screen_w;
	percent_h = (height * 100) / screen_h;
	g_debug ("window coverage x:%u%% y:%u%%", percent_w, percent_h);

	if (percent_w > GPK_SMALL_FORM_FACTOR_SCREEN_PERCENT ||
	    percent_h > GPK_SMALL_FORM_FACTOR_SCREEN_PERCENT) {
		g_debug ("using small form factor mode as %ux%u and requested %ux%u",
			   screen_w, screen_h, width, height);
		gtk_window_maximize (window);
		small_form_factor_mode = TRUE;
		goto out;
	}
#else
	/* skip invalid values */
	if (width == 0 || height == 0)
		goto out;
#endif
	/* normal size laptop panel */
	g_debug ("using native mode: %ux%u", width, height);
	gtk_window_set_default_size (window, width, height);
	small_form_factor_mode = FALSE;
out:
	return !small_form_factor_mode;
}

/**
 * gpk_window_set_parent_xid:
 **/
gboolean
gpk_window_set_parent_xid (GtkWindow *window, guint32 xid)
{
	GdkDisplay *display;
	GdkWindow *parent_window;
	GdkWindow *our_window;

	g_return_val_if_fail (xid != 0, FALSE);

	display = gdk_display_get_default ();
	parent_window = gdk_x11_window_foreign_new_for_display (display, xid);
	if (!parent_window) {
		g_warning ("Failed to create foreign window for XID %u", xid);
		return FALSE;
	}

	gtk_widget_realize (GTK_WIDGET (window));

	our_window = gtk_widget_get_window (GTK_WIDGET (window));

	/* set this above our parent */
	gdk_window_set_transient_for (our_window, parent_window);

	gtk_window_set_modal (window, TRUE);
	gtk_window_set_destroy_with_parent (GTK_WINDOW (window), TRUE);
	gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER_ON_PARENT);

	return TRUE;
}

static gchar *
gpk_package_id_get_part (const gchar *package_id, guint part)
{
	gchar **split = NULL;
	gchar *string_part = NULL;

	split = pk_package_id_split (package_id);
	string_part = g_strdup(split[part]);
	g_strfreev (split);

	return string_part;
}

gchar *
gpk_package_id_get_name (const gchar *package_id)
{
	return gpk_package_id_get_part (package_id, PK_PACKAGE_ID_NAME);
}

/**
 * gpk_get_pretty_arch:
 **/
static const gchar *
gpk_get_pretty_arch (const gchar *arch)
{
	const gchar *id = NULL;

	if (arch[0] == '\0')
		goto out;

	/* 32 bit */
	if (g_str_has_prefix (arch, "i")) {
		/* TRANSLATORS: a 32 bit package */
		id = _("32-bit");
		goto out;
	}

	/* 64 bit */
	if (g_str_has_suffix (arch, "64")) {
		/* TRANSLATORS: a 64 bit package */
		id = _("64-bit");
		goto out;
	}
out:
	return id;
}

/**
 * gpk_package_id_format_twoline:
 *
 * Return value: "<b>GTK Toolkit</b>\ngtk2-2.12.2 (i386)"
 **/
gchar *
gpk_common_format_details (const gchar *summary,
                           const gchar *details,
                           gboolean     twoline)
{
	gchar *text = NULL;

	g_return_val_if_fail (summary != NULL, NULL);

	if (details) {
		text = g_markup_printf_escaped ("%s%s<span weight=\"light\">%s</span>",
		                                summary,
		                                twoline ? "\n" : " - ",
		                                details);
	} else {
		text =  g_markup_printf_escaped ("%s", summary);
	}

	return text;
}


/**
 * gpk_package_id_format_twoline:
 *
 * Return value: "<b>GTK Toolkit</b>\ngtk2-2.12.2 (i386)"
 **/
gchar *
gpk_package_id_format_details (const gchar *package_id,
                               const gchar *summary,
                               gboolean      twoline)
{
	gchar *summary_safe = NULL;
	gchar *text = NULL;
	GString *string;
	gchar **split = NULL;
	const gchar *arch;

	g_return_val_if_fail (package_id != NULL, NULL);

	/* optional */
	split = pk_package_id_split (package_id);
	if (split == NULL) {
		g_warning ("could not parse %s", package_id);
		goto out;
	}

	/* no summary */
	if (summary == NULL || summary[0] == '\0') {
		string = g_string_new (split[PK_PACKAGE_ID_NAME]);
		if (split[PK_PACKAGE_ID_VERSION][0] != '\0')
			g_string_append_printf (string, "-%s", split[PK_PACKAGE_ID_VERSION]);
		arch = gpk_get_pretty_arch (split[PK_PACKAGE_ID_ARCH]);
		if (arch != NULL)
			g_string_append_printf (string, " (%s)", arch);
		text = g_string_free (string, FALSE);
		goto out;
	}

	/* name and summary */
	summary_safe = g_markup_escape_text (summary, -1);

	string = g_string_new ("");
	g_string_append (string, split[PK_PACKAGE_ID_NAME]);
	if (split[PK_PACKAGE_ID_VERSION][0] != '\0')
		g_string_append_printf (string, "-%s", split[PK_PACKAGE_ID_VERSION]);
	arch = gpk_get_pretty_arch (split[PK_PACKAGE_ID_ARCH]);
	if (arch != NULL)
		g_string_append_printf (string, " (%s)", arch);

	text = gpk_common_format_details (summary_safe,
	                                  g_string_free (string, FALSE),
	                                  twoline);

out:
	g_free (summary_safe);
	g_strfreev (split);

	return text;
}

/**
 * gpk_package_id_format_oneline:
 *
 * Return value: "<b>GTK Toolkit</b> (gtk2)"
 **/
gchar *
gpk_package_id_format_oneline (const gchar *package_id, const gchar *summary)
{
	gchar *summary_safe;
	gchar *text;
	gchar **split;

	g_return_val_if_fail (package_id != NULL, NULL);

	split = pk_package_id_split (package_id);
	if (summary == NULL || summary[0] == '\0') {
		/* just have name */
		text = g_strdup (split[PK_PACKAGE_ID_NAME]);
	} else {
		summary_safe = g_markup_escape_text (summary, -1);
		text = g_strdup_printf ("<b>%s</b> (%s)", summary_safe, split[PK_PACKAGE_ID_NAME]);
		g_free (summary_safe);
	}
	g_strfreev (split);
	return text;
}

/**
 * gpk_package_id_format_pretty:
 *
 * Return value: "gtk2-2.12.2 (i386)"
 **/
gchar *
gpk_package_id_format_pretty (const gchar *package_id)
{
	gchar *text = NULL;
	GString *string;
	gchar **split = NULL;
	const gchar *arch;

	g_return_val_if_fail (package_id != NULL, NULL);

	/* optional */
	split = pk_package_id_split (package_id);
	if (split == NULL) {
		g_warning ("could not parse %s", package_id);
		return NULL;
	}

	string = g_string_new (split[PK_PACKAGE_ID_NAME]);
	if (split[PK_PACKAGE_ID_VERSION][0] != '\0')
		g_string_append_printf (string, "-%s", split[PK_PACKAGE_ID_VERSION]);
	arch = gpk_get_pretty_arch (split[PK_PACKAGE_ID_ARCH]);
	if (arch != NULL)
		g_string_append_printf (string, " (%s)", arch);
	text = g_string_free (string, FALSE);

	g_strfreev (split);

	return text;
}

/**
 * gpk_check_privileged_user
 **/
gboolean
gpk_check_privileged_user (const gchar *application_name, gboolean show_ui)
{
	guint uid;
	gboolean ret = TRUE;
	gchar *message = NULL;
	gchar *title = NULL;
	GtkResponseType result;
	GtkWidget *dialog;

	uid = getuid ();
	if (uid == 0) {
		if (!show_ui)
			goto out;
		if (application_name == NULL)
			/* TRANSLATORS: these tools cannot run as root (unknown name) */
			title = g_strdup (_("This application is running as a privileged user"));
		else
			/* TRANSLATORS: cannot run as root user, and we display the application name */
			title = g_strdup_printf (_("%s is running as a privileged user"), application_name);
		message = g_strjoin ("\n",
				     /* TRANSLATORS: tell the user off */
				     _("Package management applications are security sensitive."),
				     /* TRANSLATORS: and explain why */
				     _("Running graphical applications as a privileged user should be avoided for security reasons."), NULL);

		/* give the user a choice */
		dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL,
						 GTK_MESSAGE_WARNING, GTK_BUTTONS_CANCEL, "%s", title);
		/* TRANSLATORS: button: allow the user to run this, even when insecure */
		gtk_dialog_add_button (GTK_DIALOG(dialog), _("Continue _Anyway"), GTK_RESPONSE_OK);
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG(dialog), "%s", message);
		gtk_window_set_icon_name (GTK_WINDOW(dialog), GPK_ICON_SOFTWARE_INSTALLER);
		result = gtk_dialog_run (GTK_DIALOG(dialog));
		gtk_widget_destroy (dialog);

		/* user did not agree to run insecure */
		if (result != GTK_RESPONSE_OK) {
			ret = FALSE;
			g_warning ("uid=%u so closing", uid);
			goto out;
		}
	}
out:
	g_free (title);
	g_free (message);
	return ret;
}

/**
 * gpk_time_to_imprecise_string:
 * @time_secs: The time value to convert in seconds
 *
 * Returns a localized timestring
 *
 * Return value: The time string, e.g. "2 hours"
 **/
gchar *
gpk_time_to_imprecise_string (guint time_secs)
{
	gchar* timestring = NULL;
	guint hours;
	guint minutes;
	guint seconds;

	/* is valid? */
	if (time_secs == 0) {
		/* TRANSLATORS: The actions has just literally happened */
		timestring = g_strdup_printf (_("Now"));
		goto out;
	}

	/* make local copy */
	seconds = time_secs;

	/* less than a minute */
	if (seconds < 60) {
		/* TRANSLATORS: time */
		timestring = g_strdup_printf (ngettext ("%u second", "%u seconds", seconds), seconds);
		goto out;
	}

	/* Add 0.5 to do rounding */
	minutes = (guint) ((time_secs / 60.0 ) + 0.5);

	/* less than an hour */
	if (minutes < 60) {
		/* TRANSLATORS: time */
		timestring = g_strdup_printf (ngettext ("%u minute", "%u minutes", minutes), minutes);
		goto out;
	}

	hours = minutes / 60;
	/* TRANSLATORS: time */
	timestring = g_strdup_printf (ngettext ("%u hour", "%u hours", hours), hours);
out:
	return timestring;
}

/**
 * gpk_time_ago_to_localised_string:
 * @seconds_ago: The time value to convert in seconds
 *
 * Converts a time to a string such as "5 minutes ago" or "2 weeks ago"
 *
 * Returns: (transfer full): the time string
 **/
gchar *
gpk_time_ago_to_localised_string (guint seconds_ago)
{
	gint minutes_ago, hours_ago, days_ago;
	gint weeks_ago, months_ago, years_ago;

	minutes_ago = (gint) (seconds_ago / 60);
	hours_ago = (gint) (minutes_ago / 60);
	days_ago = (gint) (hours_ago / 24);
	weeks_ago = days_ago / 7;
	months_ago = days_ago / 30;
	years_ago = weeks_ago / 52;

	if (minutes_ago < 5) {
		/* TRANSLATORS: something happened less than 5 minutes ago */
		return g_strdup (_("A moment ago"));
	} else if (hours_ago < 1)
		return g_strdup_printf (ngettext ("%d minute ago",
		                                  "%d minutes ago",
		                                  minutes_ago),
		                        minutes_ago);
	else if (days_ago < 1)
		return g_strdup_printf (ngettext ("%d hour ago",
		                                  "%d hours ago",
		                                  hours_ago),
		                        hours_ago);
	else if (days_ago < 15)
		return g_strdup_printf (ngettext ("%d day ago",
		                                  "%d days ago",
		                                  days_ago),
		                        days_ago);
	else if (weeks_ago < 8)
		return g_strdup_printf (ngettext ("%d week ago",
		                                  "%d weeks ago",
		                                  weeks_ago),
		                        weeks_ago);
	else if (years_ago < 1)
		return g_strdup_printf (ngettext ("%d month ago",
		                                  "%d months ago",
		                                  months_ago),
		                        months_ago);
	else
		return g_strdup_printf (ngettext ("%d year ago",
		                                  "%d years ago",
		                                  years_ago),
		                        years_ago);
}

/**
 * gpk_strv_join_locale:
 *
 * Return value: "dave" or "dave and john" or "dave, john and alice",
 * or %NULL for no match
 **/
gchar *
gpk_strv_join_locale (gchar **array)
{
	guint length;

	/* trivial case */
	length = g_strv_length (array);
	if (length == 0)
		return g_strdup ("none");

	/* try and get a print format */
	if (length == 1)
		return g_strdup (array[0]);
	else if (length == 2)
                /* Translators: a list of two things */
		return g_strdup_printf (_("%s and %s"),
					array[0], array[1]);
	else if (length == 3)
                /* Translators: a list of three things */
		return g_strdup_printf (_("%s, %s and %s"),
					array[0], array[1], array[2]);
	else if (length == 4)
                /* Translators: a list of four things */
		return g_strdup_printf (_("%s, %s, %s and %s"),
					array[0], array[1],
					array[2], array[3]);
	else if (length == 5)
                /* Translators: a list of five things */
		return g_strdup_printf (_("%s, %s, %s, %s and %s"),
					array[0], array[1], array[2],
					array[3], array[4]);
	return NULL;
}
