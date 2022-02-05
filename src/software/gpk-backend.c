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

#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

#include <common/gpk-task.h>

#include "gpk-backend.h"

static gboolean
_g_strzero (const gchar *text)
{
	if (text == NULL)
		return TRUE;
	if (text[0] == '\0')
		return TRUE;
	return FALSE;
}

static void     gpk_backend_finalize	(GObject	  *object);

struct _GpkBackend
{
	GObject			 parent;

	PkTask			*task;

	GHashTable		*repos;
};

G_DEFINE_TYPE (GpkBackend, gpk_backend, G_TYPE_OBJECT)

static void
gpk_backend_open_threaded (GTask        *task,
                           gpointer      source_object,
                           gpointer      task_data,
                           GCancellable *cancellable)
{
	GpkBackend *backend = NULL;
	PkResults *results = NULL;
	PkRepoDetail *item = NULL;
	GPtrArray *array = NULL;
	GError *error = NULL;
	gchar *repo_id = NULL;
	gchar *description = NULL;
	guint i;

	backend = GPK_BACKEND (source_object);

	/* get repos, so we can show the full name in the package source box */
	results = pk_client_get_repo_list (PK_CLIENT (backend->task),
	                                   pk_bitfield_value (PK_FILTER_ENUM_NONE),
	                                   cancellable,
	                                   NULL, NULL,
	                                   &error);

	if (error) {
		g_task_return_error (task, g_steal_pointer (&error));
		return;
	}

	array = pk_results_get_repo_detail_array (results);
	for (i = 0; i < array->len; i++) {
		item = g_ptr_array_index (array, i);
		g_object_get (item,
		             "repo-id", &repo_id,
		             "description", &description,
		             NULL);

		g_debug ("repo loaded = %s:%s", repo_id, description);
		/* no problem, just no point adding as we will fallback to the repo_id */
		if (description != NULL)
			g_hash_table_insert (backend->repos, g_strdup (repo_id), g_strdup (description));

		g_free (repo_id);
		g_free (description);
	}

	// FIXME: Remove fake wait to test...
	sleep (2);

	g_ptr_array_unref (array);
	g_object_unref (results);

	g_task_return_boolean (task, TRUE);
}

const gchar *
gpk_backend_get_full_repo_name (GpkBackend *backend, const gchar *repo_id)
{
	const gchar *repo_name;

	if (_g_strzero (repo_id)) {
		/* TRANSLATORS: the repo name is invalid or not found, fall back to this */
		return _("Invalid");
	}

	/* trim prefix */
	if (g_str_has_prefix (repo_id, "installed:"))
		repo_id += 10;

	/* try to find in cached repo array */
	repo_name = (const gchar *) g_hash_table_lookup (backend->repos, repo_id);
	if (repo_name == NULL) {
		g_warning ("no repo name, falling back to %s", repo_id);
		return repo_id;
	}

	return repo_name;
}

PkTask *
gpk_backend_get_task (GpkBackend *backend)
{
	g_return_val_if_fail (GPK_IS_BACKEND (backend), NULL);
	return backend->task;

}

gboolean
gpk_backend_open_finish (GpkBackend    *backend,
                         GAsyncResult  *result,
                         GError       **error)
{
	g_return_val_if_fail (GPK_IS_BACKEND (backend), FALSE);

	return g_task_propagate_boolean (G_TASK (result), error);
}

void
gpk_backend_open (GpkBackend          *backend,
                  GCancellable        *cancellable,
                  GAsyncReadyCallback  ready_callback,
                  gpointer             user_data)
{
	GTask *task = NULL;

	g_return_if_fail (GPK_IS_BACKEND (backend));
	g_return_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable));

	task = g_task_new (backend, cancellable, ready_callback, user_data);
	g_task_set_source_tag (task, gpk_backend_open);
	g_task_run_in_thread (task, gpk_backend_open_threaded);

	g_object_unref (task);
}

static void
gpk_backend_finalize (GObject *object)
{
	GpkBackend *backend;

	g_return_if_fail (GPK_IS_BACKEND (object));

	backend = GPK_BACKEND (object);

	if (backend->task != NULL)
		g_object_unref (backend->task);

	if (backend->repos != NULL)
		g_hash_table_destroy (backend->repos);

	G_OBJECT_CLASS (gpk_backend_parent_class)->finalize (object);
}

static void
gpk_backend_init (GpkBackend *backend)
{
	/* this is what we use mainly */
	backend->task = PK_TASK (gpk_task_new ());
	g_object_set (backend->task,
	              "background", FALSE,
	              NULL);

	backend->repos = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
}

static void
gpk_backend_class_init (GpkBackendClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gpk_backend_finalize;
}

GpkBackend *
gpk_backend_new (void)
{
	GpkBackend *backend;
	backend = g_object_new (GPK_TYPE_BACKEND, NULL);
	return GPK_BACKEND (backend);
}
