/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2021 Matias De lellis <mati86dl@gmail.com>
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "config.h"

#include <glib/gi18n.h>

#include <common/gpk-common.h>

#include "gpk-updates-shared.h"

#include "gpk-updates-checker.h"

struct _GpkUpdatesChecker
{
	GObject			_parent;

	GPtrArray		*update_packages;

	GpkUpdatesShared	*shared;
};

enum {
	HAS_UPDATES,
	ERROR_CHECKING,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GpkUpdatesChecker, gpk_updates_checker, G_TYPE_OBJECT)


/*
 * Search for updates.
 */

static void
gpk_updates_checker_pk_check_updates_finished_cb (GObject           *object,
                                                  GAsyncResult      *res,
                                                  GpkUpdatesChecker *checker)
{
	PkResults *results;
	GError *error = NULL;
	PkError *error_code = NULL;

	PkClient *client = PK_CLIENT(object);

	/* get the results */
	results = pk_client_generic_finish (PK_CLIENT(client), res, &error);
	if (results == NULL) {
		if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
			g_error_free (error);
			return;
		}
		g_warning ("failed to get updates: %s", error->message);
		g_error_free (error);
		g_signal_emit (checker, signals [ERROR_CHECKING], 0);
		goto out;
	}

	/* check error code */
	error_code = pk_results_get_error_code (results);
	if (error_code != NULL) {
		g_warning ("failed to get updates: %s, %s",
		           pk_error_enum_to_string (pk_error_get_code (error_code)),
		           pk_error_get_details (error_code));
		switch (pk_error_get_code (error_code)) {
			case PK_ERROR_ENUM_CANCELLED_PRIORITY:
			case PK_ERROR_ENUM_TRANSACTION_CANCELLED:
				g_debug ("ignoring error");
				break;
			default:
				g_signal_emit (checker, signals [ERROR_CHECKING], 0);
				break;
		}
		goto out;
	}

	/* so we can download or check for important & security updates */
	if (checker->update_packages != NULL)
		g_ptr_array_unref (checker->update_packages);
	checker->update_packages = pk_results_get_package_array (results);

	/* we have no updates */
	if (checker->update_packages->len == 0) {
		g_debug ("no updates");
		goto out;
	}

	/* Has updates */
	g_debug ("has updates");
	g_signal_emit (checker, signals [HAS_UPDATES], 0);

out:
	if (error_code != NULL)
		g_object_unref (error_code);
	if (results != NULL)
		g_object_unref (results);
}

static void
gpk_updates_checker_pk_check_updates (GpkUpdatesChecker *checker)
{
	/* optimize the amount of downloaded data by setting the cache age */
	pk_client_set_cache_age (PK_CLIENT(gpk_updates_shared_get_pk_task (checker->shared)),
	                         g_settings_get_int (gpk_updates_shared_get_settings (checker->shared),
	                                             GPK_SETTINGS_FREQUENCY_GET_UPDATES));

	/* get new update list */
	pk_client_get_updates_async (PK_CLIENT(gpk_updates_shared_get_pk_task (checker->shared)),
	                             pk_bitfield_value (PK_FILTER_ENUM_NONE),
	                             gpk_updates_shared_get_cancellable (checker->shared),
	                             NULL, NULL,
	                             (GAsyncReadyCallback) gpk_updates_checker_pk_check_updates_finished_cb,
	                             checker);
}

guint
gpk_updates_checker_get_important_updates_count (GpkUpdatesChecker *checker)
{
	PkPackage *pkg;
	guint i, important_packages = 0;

	for (i = 0; i < checker->update_packages->len; i++) {
		pkg = g_ptr_array_index (checker->update_packages, i);
		if (pk_package_get_info (pkg) == PK_INFO_ENUM_SECURITY ||
		    pk_package_get_info (pkg) == PK_INFO_ENUM_IMPORTANT)
			important_packages++;
	}

	return important_packages;
}

guint
gpk_updates_checker_get_updates_count (GpkUpdatesChecker *checker)
{
	return checker->update_packages->len;
}

gchar **
gpk_updates_checker_get_update_packages_ids (GpkUpdatesChecker *checker)
{
	PkPackage *pkg;
	gchar **package_ids;
	guint i;

	package_ids = g_new0 (gchar *, checker->update_packages->len + 1);
	for (i = 0; i < checker->update_packages->len; i++) {
		pkg = g_ptr_array_index (checker->update_packages, i);
		package_ids[i] = g_strdup (pk_package_get_id (pkg));
	}

	return package_ids;
}

void
gpk_updates_checker_check_for_updates (GpkUpdatesChecker *checker)
{
	gpk_updates_checker_pk_check_updates (checker);
}

static void
gpk_updates_checker_dispose (GObject *object)
{
	GpkUpdatesChecker *checker;

	checker = GPK_UPDATES_CHECKER (object);

	g_debug ("Stopping updates checker");

	g_clear_object (&checker->shared);

	if (checker->update_packages != NULL)
		g_ptr_array_unref (checker->update_packages);

	g_debug ("Stopped pdates checker");

	G_OBJECT_CLASS (gpk_updates_checker_parent_class)->dispose (object);
}

static void
gpk_updates_checker_class_init (GpkUpdatesCheckerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gpk_updates_checker_dispose;

	signals [HAS_UPDATES] =
		g_signal_new ("has-updates",
		              G_TYPE_FROM_CLASS (object_class), G_SIGNAL_RUN_LAST,
		              0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0);

	signals [ERROR_CHECKING] =
		g_signal_new ("error-checking",
		              G_TYPE_FROM_CLASS (object_class), G_SIGNAL_RUN_LAST,
		              0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0);
}

static void
gpk_updates_checker_init (GpkUpdatesChecker *checker)
{
	g_debug ("Starting updates checker");

	/* The shared code between the different tasks */
	checker->shared = gpk_updates_shared_get ();

	g_debug ("Started updates checker");
}

GpkUpdatesChecker *
gpk_updates_checker_new (void)
{
	GpkUpdatesChecker *checker;
	checker = g_object_new (GPK_TYPE_UPDATES_CHECKER, NULL);
	return GPK_UPDATES_CHECKER (checker);
}

