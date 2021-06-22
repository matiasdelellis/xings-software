/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2021 Matias De lellis <mati86dl@gmail.com>
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

#include <glib/gi18n.h>
#include <packagekit-glib2/packagekit.h>

#include "gpk-common.h"

#include "gpk-scheduler.h"

#define PERIODIC_CHECK_TIME	60*60
#define DEFERRING_TIMEOUT	10

struct _GpkScheduler
{
	GObject			_parent;

	GSettings		*settings;
	PkControl		*control;

	gboolean		 network_active;

	guint			 timeout_id;
	guint			 periodic_id;
};

enum {
	REFRESH_CACHE,
	GET_UPDATES,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GpkScheduler, gpk_scheduler, G_TYPE_OBJECT)

static gboolean
pk_network_enum_is_online (PkNetworkEnum state)
{
	/* offline or mobile. */
	if (state == PK_NETWORK_ENUM_OFFLINE ||
	    state == PK_NETWORK_ENUM_MOBILE)
		return FALSE;

	/* online */
	if (state == PK_NETWORK_ENUM_ONLINE ||
	    state == PK_NETWORK_ENUM_WIFI ||
	    state == PK_NETWORK_ENUM_WIRED)
		return TRUE;

	/* not recognised */
	g_warning ("unknown state: %i", state);

	return TRUE;
}

static void
gpk_scheduler_get_time_since_refresh_cache_cb (GObject      *object,
                                               GAsyncResult *res,
                                               GpkScheduler *scheduler)
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
	threshold = g_settings_get_int (scheduler->settings,
	                                GPK_SETTINGS_FREQUENCY_GET_UPDATES);
	if (seconds < threshold) {
		g_debug ("not before timeout, thresh=%u, now=%u", threshold, seconds);
		return;
	}

	/* send signal */
	g_debug ("emitting refresh-cache");
	g_signal_emit (scheduler, signals [REFRESH_CACHE], 0);
}

static void
gpk_scheduler_check_refresh_cache (GpkScheduler *scheduler)
{
	guint threshold;

	g_return_if_fail (GPK_IS_SCHEDULER (scheduler));

	/* if we don't want to auto check for updates, don't do this either */
	threshold = g_settings_get_int (scheduler->settings,
	                                GPK_SETTINGS_FREQUENCY_GET_UPDATES);
	if (threshold == 0) {
		g_debug ("not when policy is set to never");
		return;
	}

	/* get this each time, as it may have changed behind out back */
	threshold = g_settings_get_int (scheduler->settings,
	                                GPK_SETTINGS_FREQUENCY_REFRESH_CACHE);
	if (threshold == 0) {
		g_debug ("not when policy is set to never");
		return;
	}

	/* get the time since the last scheduler */
	pk_control_get_time_since_action_async (scheduler->control,
	                                        PK_ROLE_ENUM_REFRESH_CACHE,
	                                        NULL,
	                                        (GAsyncReadyCallback) gpk_scheduler_get_time_since_refresh_cache_cb,
	                                        scheduler);
}

static void
gpk_scheduler_get_time_since_get_updates_cb (GObject      *object,
                                             GAsyncResult *res,
                                             GpkScheduler *scheduler)
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

	/* have we passed the timout? */
	threshold = g_settings_get_int (scheduler->settings,
	                                GPK_SETTINGS_FREQUENCY_GET_UPDATES);
	if (seconds < threshold) {
		g_debug ("not before timeout, thresh=%u, now=%u", threshold, seconds);
		return;
	}

	/* send signal */
	g_debug ("emitting get-updates");
	g_signal_emit (scheduler, signals [GET_UPDATES], 0);
}

static void
gpk_scheduler_check_get_updates (GpkScheduler *scheduler)
{
	guint threshold;

	g_return_if_fail (GPK_IS_SCHEDULER (scheduler));

	/* if we don't want to auto check for updates, don't do this either */
	threshold = g_settings_get_int (scheduler->settings,
	                                GPK_SETTINGS_FREQUENCY_GET_UPDATES);
	if (threshold == 0) {
		g_debug ("not when policy is set to never");
		return;
	}

	/* get the time since the last scheduler */
	pk_control_get_time_since_action_async (scheduler->control,
	                                        PK_ROLE_ENUM_GET_UPDATES,
	                                        NULL,
	                                        (GAsyncReadyCallback) gpk_scheduler_get_time_since_get_updates_cb,
	                                        scheduler);
}

static gboolean
gpk_scheduler_program_status_check_cb (GpkScheduler *scheduler)
{
	/* check all actions */
	gpk_scheduler_check_refresh_cache (scheduler);
	gpk_scheduler_check_get_updates (scheduler);

	return FALSE;
}

static gboolean
gpk_scheduler_program_status_check (GpkScheduler *scheduler)
{
	g_return_val_if_fail (GPK_IS_SCHEDULER (scheduler), FALSE);

	/* no point continuing if we have no network */
	if (!scheduler->network_active) {
		g_debug ("not when no network");
		return FALSE;
	}

	/* wait a little time for things to settle down */
	if (scheduler->timeout_id != 0)
		g_source_remove (scheduler->timeout_id);
	g_debug ("defering action for %i seconds", DEFERRING_TIMEOUT);

	scheduler->timeout_id =
		g_timeout_add_seconds (DEFERRING_TIMEOUT,
		                       (GSourceFunc) gpk_scheduler_program_status_check_cb,
		                       scheduler);
	g_source_set_name_by_id (scheduler->timeout_id,
	                         "[GpkScheduler] change-state");

	return TRUE;
}

static void
gpk_scheduler_notify_network_state_cb (PkControl    *control,
                                       GParamSpec   *pspec,
                                       GpkScheduler *scheduler)
{
	PkNetworkEnum state;

	g_return_if_fail (GPK_IS_SCHEDULER (scheduler));

	g_object_get (control, "network-state", &state, NULL);
	scheduler->network_active = pk_network_enum_is_online (state);
	g_debug ("setting online %i", scheduler->network_active);

	if (scheduler->network_active)
		gpk_scheduler_program_status_check (scheduler);
}

static void
gpk_scheduler_get_control_properties_cb (GObject      *object,
                                         GAsyncResult *res,
                                         GpkScheduler *scheduler)
{
	PkNetworkEnum state;
	GError *error = NULL;
	gboolean ret;

	PkControl *control = PK_CONTROL(object);

	/* get the result */
	ret = pk_control_get_properties_finish (control, res, &error);
	if (!ret) {
		g_warning ("could not get properties");
		g_error_free (error);
		goto out;
	}

	/* get values */
	g_object_get (control, "network-state", &state, NULL);
	scheduler->network_active = pk_network_enum_is_online (state);

out:
	return;
}

static gboolean
gpk_scheduler_periodic_refresh_timeout_cb (gpointer user_data)
{
	GpkScheduler *scheduler = GPK_SCHEDULER (user_data);

	g_return_val_if_fail (GPK_IS_SCHEDULER (scheduler), FALSE);

	/* debug so we can catch polling */
	g_debug ("polling check");

	/* triggered once an hour */
	gpk_scheduler_program_status_check (scheduler);

	/* always return */
	return G_SOURCE_CONTINUE;
}

static void
gpk_scheduler_init (GpkScheduler *scheduler)
{
	GVariant *status;
	guint status_code;

	scheduler->network_active = FALSE;
	scheduler->timeout_id = 0;
	scheduler->periodic_id = 0;

	/* we need to know the updates frequency */
	scheduler->settings = g_settings_new (GPK_SETTINGS_SCHEMA);

	/* we need to query the last cache scheduler time */
	scheduler->control = pk_control_new ();
	g_signal_connect (scheduler->control, "notify::network-state",
	                  G_CALLBACK (gpk_scheduler_notify_network_state_cb),
	                  scheduler);

	/* get network state */
	pk_control_get_properties_async (scheduler->control,
	                                 NULL,
	                                 (GAsyncReadyCallback) gpk_scheduler_get_control_properties_cb,
	                                 scheduler);

	/* we check this in case we miss one of the async signals */
	scheduler->periodic_id =
		g_timeout_add_seconds (PERIODIC_CHECK_TIME,
		                       gpk_scheduler_periodic_refresh_timeout_cb,
		                       scheduler);
	g_source_set_name_by_id (scheduler->periodic_id,
	                         "[GpkScheduler] periodic check");

	/* check system state */
	gpk_scheduler_program_status_check (scheduler);
}

static void
gpk_scheduler_finalize (GObject *object)
{
	GpkScheduler *scheduler;

	g_return_if_fail (GPK_IS_SCHEDULER (object));

	scheduler = GPK_SCHEDULER (object);

	if (scheduler->timeout_id != 0)
		g_source_remove (scheduler->timeout_id);
	if (scheduler->periodic_id != 0)
		g_source_remove (scheduler->periodic_id);

	g_object_unref (scheduler->control);
	g_object_unref (scheduler->settings);

	G_OBJECT_CLASS (gpk_scheduler_parent_class)->finalize (object);
}

static void
gpk_scheduler_class_init (GpkSchedulerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gpk_scheduler_finalize;

	signals [REFRESH_CACHE] =
		g_signal_new ("refresh-cache",
		              G_TYPE_FROM_CLASS (object_class), G_SIGNAL_RUN_LAST,
		              0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0);
	signals [GET_UPDATES] =
		g_signal_new ("get-updates",
		              G_TYPE_FROM_CLASS (object_class), G_SIGNAL_RUN_LAST,
		              0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0);
}

GpkScheduler *
gpk_scheduler_new (void)
{
	GpkScheduler *scheduler;
	scheduler = g_object_new (GPK_TYPE_SCHEDULER, NULL);
	return GPK_SCHEDULER (scheduler);
}

