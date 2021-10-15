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

#include "gpk-updates-download.h"

struct _GpkUpdatesDownload
{
	GObject			_parent;

	GpkUpdatesShared	*shared;
};

enum {
	DOWNLOAD_DONE,
	ERROR_DOWNLOADING,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GpkUpdatesDownload, gpk_updates_download, G_TYPE_OBJECT)

static void
gpk_updates_download_pk_download_finished_cb (GObject            *object,
                                              GAsyncResult       *res,
                                              GpkUpdatesDownload *download)
{
	PkClient *client = PK_CLIENT(object);
	PkResults *results;
	PkError *error_code = NULL;
	GError *error = NULL;

	/* get the results */
	results = pk_client_generic_finish (PK_CLIENT(client), res, &error);
	if (results == NULL) {
		if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
			g_error_free (error);
			return;
		}
		g_warning ("failed to download: %s", error->message);
		g_error_free (error);
		g_signal_emit (download, signals [ERROR_DOWNLOADING], 0);
		return;
	}

	/* check error code */
	error_code = pk_results_get_error_code (results);
	if (error_code != NULL) {
		g_warning ("failed to download: %s, %s",
		           pk_error_enum_to_string (pk_error_get_code (error_code)),
		           pk_error_get_details (error_code));
		switch (pk_error_get_code (error_code)) {
			case PK_ERROR_ENUM_CANCELLED_PRIORITY:
			case PK_ERROR_ENUM_TRANSACTION_CANCELLED:
				g_debug ("ignoring error");
				break;
			default:
				g_signal_emit (download, signals [ERROR_DOWNLOADING], 0);
				break;
		}
		goto out;
	}

	g_debug ("updates downloaded");
	g_signal_emit (download, signals [DOWNLOAD_DONE], 0);

out:
	if (error_code != NULL)
		g_object_unref (error_code);
	if (results != NULL)
		g_object_unref (results);
}

static void
gpk_updates_download_pk_auto_download_updates (GpkUpdatesDownload *download, gchar **package_ids)
{
	/* we've set only-download in PkTask */
	pk_task_update_packages_async (gpk_updates_shared_get_pk_task (download->shared),
	                               package_ids,
	                               gpk_updates_shared_get_cancellable (download->shared),
	                               NULL, NULL,
	                               (GAsyncReadyCallback) gpk_updates_download_pk_download_finished_cb,
	                               download);
}

static void
gpk_updates_download_dispose (GObject *object)
{
	GpkUpdatesDownload *download;

	download = GPK_UPDATES_DOWNLOAD (object);

	g_debug ("Stopping updates download");

	g_clear_object (&download->shared);

	g_debug ("Stopped pdates download");

	G_OBJECT_CLASS (gpk_updates_download_parent_class)->dispose (object);
}

static void
gpk_updates_download_class_init (GpkUpdatesDownloadClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gpk_updates_download_dispose;

	signals [DOWNLOAD_DONE] =
		g_signal_new ("download-done",
		              G_TYPE_FROM_CLASS (object_class), G_SIGNAL_RUN_LAST,
		              0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0);

	signals [ERROR_DOWNLOADING] =
		g_signal_new ("error-downloading",
		              G_TYPE_FROM_CLASS (object_class), G_SIGNAL_RUN_LAST,
		              0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0);
}

static void
gpk_updates_download_init (GpkUpdatesDownload *download)
{
	g_debug ("Starting updates download");

	/* The shared code between the different tasks */
	download->shared = gpk_updates_shared_get ();

	g_debug ("Started updates download");
}

void
gpk_updates_download_auto_download_updates (GpkUpdatesDownload *download, gchar **package_ids)
{
	gpk_updates_download_pk_auto_download_updates (download, package_ids);
}

GpkUpdatesDownload *
gpk_updates_download_new (void)
{
	GpkUpdatesDownload *download;
	download = g_object_new (GPK_TYPE_UPDATES_DOWNLOAD, NULL);
	return GPK_UPDATES_DOWNLOAD (download);
}
