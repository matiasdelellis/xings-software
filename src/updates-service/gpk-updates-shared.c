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

struct _GpkUpdatesShared
{
	GObject			_parent;

	PkControl		*control;
	PkTask			*task;
	GCancellable		*cancellable;
	GSettings		*settings;
};

G_DEFINE_TYPE (GpkUpdatesShared, gpk_updates_shared, G_TYPE_OBJECT)


PkControl *
gpk_updates_shared_get_pk_control (GpkUpdatesShared *shared)
{
	return shared->control;
}

PkTask *
gpk_updates_shared_get_pk_task (GpkUpdatesShared *shared)
{
	return shared->task;
}


GCancellable *
gpk_updates_shared_get_cancellable (GpkUpdatesShared *shared)
{
	return shared->cancellable;
}

GSettings *
gpk_updates_shared_get_settings (GpkUpdatesShared *shared)
{
	return shared->settings;
}

static void
gpk_updates_shared_dispose (GObject *object)
{
	GpkUpdatesShared *shared;

	shared = GPK_UPDATES_SHARED (object);

	g_debug ("Stopping updates shared");

	if (shared->cancellable) {
		g_cancellable_cancel (shared->cancellable);
		g_clear_object (&shared->cancellable);
	}

	g_clear_object (&shared->control);
	g_clear_object (&shared->task);

	g_clear_object (&shared->settings);

	g_debug ("Stopped updates shared");

	G_OBJECT_CLASS (gpk_updates_shared_parent_class)->dispose (object);
}

static void
gpk_updates_shared_class_init (GpkUpdatesSharedClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gpk_updates_shared_dispose;
}

static void
gpk_updates_shared_init (GpkUpdatesShared *shared)
{
	g_debug ("Starting updates shared");

	/* we need to know the updates frequency */
	shared->settings = g_settings_new (GPK_SETTINGS_SCHEMA);

	/* PackageKit */
	shared->control = pk_control_new ();

	shared->task = pk_task_new ();
	g_object_set (shared->task,
	              "background", TRUE,
	              "interactive", FALSE,
	              "only-download", TRUE,
	              NULL);

	shared->cancellable = g_cancellable_new ();

	g_debug ("Started updates shared");
}

GpkUpdatesShared *
gpk_updates_shared_get (void)
{
	static GpkUpdatesShared *shared = NULL;

	if (G_UNLIKELY (shared == NULL)) {
		shared = g_object_new (GPK_TYPE_UPDATES_SHARED, NULL);
		g_object_add_weak_pointer(G_OBJECT (shared),
		                          (gpointer) &shared);
	} else {
		g_object_ref (G_OBJECT (shared));
	}

	return GPK_UPDATES_SHARED (shared);
}

