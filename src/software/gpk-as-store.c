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

#include <string.h>
#include <glib.h>

#include "gpk-as-store.h"

static void     gpk_as_store_finalize	(GObject	  *object);

struct _GpkAsStore
{
	GObject			 parent;

	AsPool			*as_pool;
	GHashTable		*packages_components;
};

G_DEFINE_TYPE (GpkAsStore, gpk_as_store, G_TYPE_OBJECT)

gboolean
gpk_as_store_load (GpkAsStore *store, GCancellable *cancellable, GError **error)
{
	gchar **pkgnames = NULL;
	GPtrArray *components = NULL;
	AsComponent *component = NULL;
	const gchar *pkgname = NULL;
	guint i = 0, p = 0;


	if (!as_pool_load (store->as_pool, cancellable, error))
		return FALSE;

	components = as_pool_get_components (store->as_pool);
	for (i = 0; i < components->len; i++) {
		component = AS_COMPONENT (g_ptr_array_index (components, i));
		pkgnames = as_component_get_pkgnames (component);
		if (pkgnames == NULL)
			continue;
		for (p = 0; pkgnames[p] != NULL; p++) {
			pkgname = pkgnames[p];
			g_hash_table_insert (store->packages_components,
			                     g_strdup(pkgname),
			                     g_object_ref (component));
		}
	}

	g_debug ("Appstream components: %u", components->len);
	g_debug ("Appstream packages: %u", g_hash_table_size(store->packages_components));

	return TRUE;
}

gchar *
gpk_as_component_get_desktop_id (AsComponent *component)
{
	AsLaunchable *launch;
	GPtrArray *entries;

	launch = as_component_get_launchable (component, AS_LAUNCHABLE_KIND_DESKTOP_ID);
	if (launch == NULL)
		return NULL;

	entries = as_launchable_get_entries (launch);
	if (entries->len <= 0)
		return NULL;

	return g_strdup((const gchar*) g_ptr_array_index (entries, 0));
}

AsComponent *
gpk_as_store_get_component_by_pkgname (GpkAsStore *store, const gchar *pkgname)
{
	return g_hash_table_lookup (store->packages_components, pkgname);
}

gchar **
gpk_as_store_search_pkgnames_by_categories (GpkAsStore *store, gchar **categories)
{
	GPtrArray *pkgname_list = NULL, *components = NULL;
	AsComponent *component = NULL;
	const gchar *pkgname = NULL;
	guint i = 0;

	pkgname_list = g_ptr_array_new ();

	components = as_pool_get_components_by_categories (store->as_pool, categories);
	for (i = 0; i < components->len; i++) {
		component = AS_COMPONENT (g_ptr_array_index (components, i));
		pkgname = as_component_get_pkgname (component);
		if (pkgname) {
			g_ptr_array_add (pkgname_list, g_strdup(pkgname));
		}
	}

	g_ptr_array_add (pkgname_list, NULL);

	return (char **) g_ptr_array_free (pkgname_list, FALSE);
}

gchar **
gpk_as_store_search_pkgnames (GpkAsStore *store, const gchar *search)
{
	GPtrArray *pkgname_list = NULL, *components = NULL;
	AsComponent *component = NULL;
	const gchar *pkgname = NULL;
	guint i = 0;

	pkgname_list = g_ptr_array_new ();

	components = as_pool_search (store->as_pool, search);
	for (i = 0; i < components->len; i++) {
		component = AS_COMPONENT (g_ptr_array_index (components, i));
		pkgname = as_component_get_pkgname (component);
		if (pkgname) {
			g_ptr_array_add (pkgname_list, g_strdup(pkgname));
		}
	}

	g_ptr_array_add (pkgname_list, NULL);

	return (char **) g_ptr_array_free (pkgname_list, FALSE);
}

static void
gpk_as_store_finalize (GObject *object)
{
	GpkAsStore *store;

	g_return_if_fail (GPK_IS_AS_STORE (object));

	store = GPK_AS_STORE (object);

	g_hash_table_unref (store->packages_components);
	g_object_unref (store->as_pool);

	G_OBJECT_CLASS (gpk_as_store_parent_class)->finalize (object);
}

static void
gpk_as_store_init (GpkAsStore *store)
{
	store->as_pool = as_pool_new ();
	store->packages_components = g_hash_table_new_full (g_str_hash, g_str_equal, (GDestroyNotify) g_free, (GDestroyNotify) g_object_unref);
}

static void
gpk_as_store_class_init (GpkAsStoreClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gpk_as_store_finalize;
}

GpkAsStore *
gpk_as_store_new (void)
{
	GpkAsStore *store;
	store = g_object_new (GPK_TYPE_AS_STORE, NULL);
	return GPK_AS_STORE (store);
}
