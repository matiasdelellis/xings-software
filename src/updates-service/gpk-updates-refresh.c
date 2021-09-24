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

#include "gpk-updates-refresh.h"
#include "gpk-updates-shared.h"

struct _GpkUpdatesRefresh
{
	GObject			_parent;

	GpkUpdatesShared	*shared;
};

enum {
	VALID_CACHE,
	ERROR_REFRESH,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GpkUpdatesRefresh, gpk_updates_refresh, G_TYPE_OBJECT)

/*
 * Refresh cache.
 */

static void
gpk_updates_refresh_pk_refresh_cache_finished_cb (GObject           *object,
                                                  GAsyncResult      *res,
                                                  GpkUpdatesRefresh *refresh)
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
		g_warning ("failed to refresh the cache: %s", error->message);
		g_error_free (error);

		g_signal_emit (refresh, signals [ERROR_REFRESH], 0);
		return;
	}

	/* check error code */
	error_code = pk_results_get_error_code (results);
	if (error_code != NULL) {
		g_warning ("failed to refresh the cache: %s, %s",
		           pk_error_enum_to_string (pk_error_get_code (error_code)),
		           pk_error_get_details (error_code));
		g_object_unref (error_code);
		g_object_unref (results);

		g_signal_emit (refresh, signals [ERROR_REFRESH], 0);

		return;
	}

	g_debug ("Cache was updated.");
	g_signal_emit (refresh, signals [VALID_CACHE], 0);
}

static void
gpk_updates_refresh_pk_refresh_cache (GpkUpdatesRefresh *refresh)
{
	/* optimize the amount of downloaded data by setting the cache age */
	pk_client_set_cache_age (PK_CLIENT(gpk_updates_shared_get_pk_task (refresh->shared)),
	                         g_settings_get_int (gpk_updates_shared_get_settings (refresh->shared),
	                                             GPK_SETTINGS_FREQUENCY_REFRESH_CACHE));

	pk_client_refresh_cache_async (PK_CLIENT(gpk_updates_shared_get_pk_task (refresh->shared)),
	                               TRUE,
	                               gpk_updates_shared_get_cancellable (refresh->shared),
	                               NULL, NULL,
	                               (GAsyncReadyCallback) gpk_updates_refresh_pk_refresh_cache_finished_cb,
	                               refresh);
}


/*
 * Scheduler the refesh cache to optimize usage.
 */

static void
gpk_updates_refresh_pk_get_time_since_refresh_cache_cb (GObject           *object,
                                                        GAsyncResult      *res,
                                                        GpkUpdatesRefresh *refresh)
{
	GError *error = NULL;
	guint seconds;
	guint threshold;

	PkControl *control = PK_CONTROL (object);

	/* get the result */
	seconds = pk_control_get_time_since_action_finish (control, res, &error);
	if (seconds == 0) {
		g_warning ("failed to get time: %s", error->message);
		g_error_free (error);
		return;
	}

	/* have we passed the timeout? */
	threshold = g_settings_get_int (gpk_updates_shared_get_settings (refresh->shared),
	                                GPK_SETTINGS_FREQUENCY_GET_UPDATES);

	if (seconds < threshold) {
		g_debug ("not refresh before timeout, thresh=%u, now=%u", threshold, seconds);
		g_signal_emit (refresh, signals [VALID_CACHE], 0);
		return;
	}

	/* send signal */
	g_debug ("must refresh cache");
	gpk_updates_refresh_pk_refresh_cache (refresh);
}

static void
gpk_updates_refresh_pk_check_refresh_cache (GpkUpdatesRefresh *refresh)
{
	guint threshold;

	g_return_if_fail (GPK_IS_UPDATES_REFRESH (refresh));

	/* if we don't want to auto check for updates, don't do this either */
	threshold = g_settings_get_int (gpk_updates_shared_get_settings (refresh->shared),
	                                GPK_SETTINGS_FREQUENCY_GET_UPDATES);
	if (threshold == 0) {
		g_debug ("not when policy is set to never");
		return;
	}

	/* get this each time, as it may have changed behind out back */
	threshold = g_settings_get_int (gpk_updates_shared_get_settings (refresh->shared),
	                                GPK_SETTINGS_FREQUENCY_REFRESH_CACHE);
	if (threshold == 0) {
		g_debug ("not when policy is set to never");
		return;
	}

	/* get the time since the last scheduler */
	pk_control_get_time_since_action_async (gpk_updates_shared_get_pk_control (refresh->shared),
	                                        PK_ROLE_ENUM_REFRESH_CACHE,
	                                        NULL,
	                                        (GAsyncReadyCallback) gpk_updates_refresh_pk_get_time_since_refresh_cache_cb,
	                                        refresh);
}


void
gpk_updates_refresh_update_cache (GpkUpdatesRefresh *refresh)
{
	gpk_updates_refresh_pk_check_refresh_cache (refresh);
}


/**
 *  GpkUpdatesRefresh:
 */

static void
gpk_updates_refresh_dispose (GObject *object)
{
	GpkUpdatesRefresh *refresh;

	refresh = GPK_UPDATES_REFRESH (object);

	g_debug ("Stopping updates refresh");

	g_clear_object (&refresh->shared);

	g_debug ("Stopped pdates refresh");

	G_OBJECT_CLASS (gpk_updates_refresh_parent_class)->dispose (object);
}

static void
gpk_updates_refresh_class_init (GpkUpdatesRefreshClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gpk_updates_refresh_dispose;

	signals [VALID_CACHE] =
		g_signal_new ("valid-cache",
		              G_TYPE_FROM_CLASS (object_class), G_SIGNAL_RUN_LAST,
		              0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0);

	signals [ERROR_REFRESH] =
		g_signal_new ("error-refresh",
		              G_TYPE_FROM_CLASS (object_class), G_SIGNAL_RUN_LAST,
		              0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0);
}

static void
gpk_updates_refresh_init (GpkUpdatesRefresh *refresh)
{
	g_debug ("Starting updates refresh");

	/* The shared code between the different tasks */
	refresh->shared = gpk_updates_shared_get ();

	g_debug ("Started updates refresh");
}

GpkUpdatesRefresh *
gpk_updates_refresh_new (void)
{
	GpkUpdatesRefresh *refresh;
	refresh = g_object_new (GPK_TYPE_UPDATES_REFRESH, NULL);
	return GPK_UPDATES_REFRESH (refresh);
}

