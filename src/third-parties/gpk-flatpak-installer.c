/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2023 Matias De lellis <mati86dl@gmail.com>
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
#include <flatpak/flatpak.h>

#include "gpk-flatpak-installer.h"
#include "gpk-flatpak-ref.h"

#define FLATPAK_CLI_UPDATE_FREQUENCY 500

static void     gpk_flatpak_installer_finalize   (GObject		*object);

typedef struct
{
	FlatpakInstallation *installation;
	GpkFlatpakRef       *flatpakref;

	guint64              download_size;
} GpkFlatpakInstallerPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GpkFlatpakInstaller, gpk_flatpak_installer, G_TYPE_OBJECT)

static void
gpk_flatpak_transaction_progress_changed_cb (FlatpakTransactionProgress *progress,
                                             gpointer data)
{
	guint percent = flatpak_transaction_progress_get_progress (progress);
	g_debug ("%u %%", percent);
}

static gboolean
gpk_flatpak_transaction_add_new_remote (FlatpakTransaction *transaction,
                                        FlatpakTransactionRemoteReason reason,
                                        const char *from_id,
                                        const char *remote_name,
                                        const char *url)
{
	g_debug ("Transaction request to add new remote: %s", remote_name);

	// Allow new remotes.
	return TRUE;
}

static void
gpk_flatpak_transaction_new_operation (FlatpakTransaction *transaction,
                                       FlatpakTransactionOperation *op,
                                       FlatpakTransactionProgress  *progress)
{
	FlatpakTransactionOperationType op_type = flatpak_transaction_operation_get_operation_type (op);
	const char *ref = flatpak_transaction_operation_get_ref (op);

	switch (op_type) {
		case FLATPAK_TRANSACTION_OPERATION_INSTALL_BUNDLE:
		case FLATPAK_TRANSACTION_OPERATION_INSTALL:
			g_debug(_("Installing %s\n"), ref);
			break;
		case FLATPAK_TRANSACTION_OPERATION_UPDATE:
			g_debug(_("Updating %s\n"), ref);
			break;
		case FLATPAK_TRANSACTION_OPERATION_UNINSTALL:
			g_debug (_("Uninstalling %s\n"), ref);
			break;
		default:
			g_assert_not_reached ();
			break;
	}

	flatpak_transaction_progress_set_update_frequency (progress, FLATPAK_CLI_UPDATE_FREQUENCY);
	g_signal_connect (progress, "changed",
	                  G_CALLBACK (gpk_flatpak_transaction_progress_changed_cb), NULL);
}

static gboolean
gpk_flatpak_transaction_ready_but_simulate (FlatpakTransaction  *transaction,
                                            GpkFlatpakInstaller *installer)
{
	GpkFlatpakInstallerPrivate *priv;
	FlatpakTransactionOperation *operation;
	GList *opertations, *l;
	gchar *human_size = NULL;

	priv = gpk_flatpak_installer_get_instance_private (installer);

	if (priv->download_size > 0)
		return TRUE;

	opertations = flatpak_transaction_get_operations (transaction);
	for (l = opertations; l != NULL; l = l->next) {
		operation = l->data;
		priv->download_size += flatpak_transaction_operation_get_download_size (operation);
	}

	human_size = g_format_size (priv->download_size);
	g_debug ("Download size: %s", human_size);
	g_free (human_size);

	// Do not allow the install since we are simulating.
	return FALSE;
}

gboolean
gpk_flatpak_installer_launch_ready (GpkFlatpakInstaller *installer, GError **error)
{
	gchar *name = NULL, *branch = NULL;

	GpkFlatpakInstallerPrivate *priv;

	priv = gpk_flatpak_installer_get_instance_private (installer);

	name = gpk_flatpak_ref_get_name(priv->flatpakref);
	branch = gpk_flatpak_ref_get_branch(priv->flatpakref);

	if (!flatpak_installation_launch (priv->installation,
	                                  name,
	                                  NULL,
	                                  branch,
	                                  NULL,
	                                  NULL,
	                                  error)) {
		g_warning ("Failed to launch Flatpak application");
		return FALSE;
	}

	g_free (name);
	g_free (branch);

	return TRUE;
}

guint64
gpk_flatpak_installer_get_download_size (GpkFlatpakInstaller *installer)
{
	GpkFlatpakInstallerPrivate *priv;
	priv = gpk_flatpak_installer_get_instance_private (installer);
	return priv->download_size;
}

gchar *
gpk_flatpak_installer_get_name (GpkFlatpakInstaller *installer)
{
	GpkFlatpakInstallerPrivate *priv;
	priv = gpk_flatpak_installer_get_instance_private (installer);
	return gpk_flatpak_ref_get_name(priv->flatpakref);
}

gboolean
gpk_flatpak_installer_perform (GpkFlatpakInstaller *installer, GError **error)
{
	FlatpakTransaction  *transaction;
	GpkFlatpakInstallerPrivate *priv;
	GBytes *raw_data = NULL;
	gboolean ret = TRUE;

	priv = gpk_flatpak_installer_get_instance_private (installer);

	/* Creates a new FlatpakTransaction to installation */
	transaction = flatpak_transaction_new_for_installation (priv->installation, NULL, NULL);

	/* Connect signals. */
	g_signal_connect (transaction, "add-new-remote",
	                  G_CALLBACK (gpk_flatpak_transaction_add_new_remote), NULL);
	g_signal_connect (transaction, "new-operation",
	                  G_CALLBACK (gpk_flatpak_transaction_new_operation), NULL);

	raw_data = gpk_flatpak_ref_get_raw_data (priv->flatpakref);

	/* Adds installing the given flatpakref to this transaction.*/
	if (!flatpak_transaction_add_install_flatpakref (transaction,
	                                                 raw_data,
	                                                 error)) {
		g_warning ("Failed to add installer transaction");
		ret = FALSE;
		goto out;
	}

	/* Executes the transaction */
	if (!flatpak_transaction_run (transaction, NULL, error)) {
		g_warning ("Failed to run Flatpak transaction");
		ret = FALSE;
		goto out;
	}

out:
	if (transaction)
		g_object_unref (transaction);

	if (raw_data)
		g_bytes_unref (raw_data);

	return ret;
}

gboolean
gpk_flatpak_installer_preprare_flatpakref (GpkFlatpakInstaller *installer, gchar *file, GError **error)
{
	FlatpakTransaction *transaction;
	GpkFlatpakInstallerPrivate *priv;
	GBytes *raw_data = NULL;
	gboolean ret = TRUE;

	priv = gpk_flatpak_installer_get_instance_private (installer);

	/* Creates a new FlatpakTransaction to installation */
	transaction = flatpak_transaction_new_for_installation (priv->installation, NULL, NULL);

	/* Connect signals. */
	g_signal_connect (transaction, "add-new-remote",
	                  G_CALLBACK (gpk_flatpak_transaction_add_new_remote), NULL);
	g_signal_connect (transaction, "new-operation",
	                  G_CALLBACK (gpk_flatpak_transaction_new_operation), NULL);
	// This signal indicates that it is ready to install, but we cancel here to simulate
	g_signal_connect (transaction, "ready",
	                  G_CALLBACK (gpk_flatpak_transaction_ready_but_simulate), installer);

	/* Create a new FlatpakRef */
	priv->flatpakref = gpk_flatpak_ref_new ();

	/* Load that.*/
	if (!gpk_flatpak_ref_load_from_file (priv->flatpakref, file, error)) {
		g_debug("Failed to load FlatpakRef file");
		ret = FALSE;
		goto out;
	}

	raw_data = gpk_flatpak_ref_get_raw_data (priv->flatpakref);

	/* Adds installing the given flatpakref to this transaction.*/
	if (!flatpak_transaction_add_install_flatpakref (transaction,
	                                                 raw_data,
	                                                 error)) {
		g_warning ("Failed to add installer transaction");
		ret = FALSE;
		goto out;
	}

	/* Executes the transaction */
	if (!flatpak_transaction_run (transaction, NULL, error)) {
		// The cancellation to simulate responds to an error
		if (g_error_matches (*error, FLATPAK_ERROR, FLATPAK_ERROR_ABORTED)) {
			g_clear_error (error);
		} else {
			g_warning ("Failed to run prepare transaction");
			ret = FALSE;
			goto out;
		}
	}

out:
	if (transaction)
		g_object_unref (transaction);

	if (raw_data)
		g_bytes_unref (raw_data);

	return ret;
}

static void
gpk_flatpak_installer_class_init (GpkFlatpakInstallerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gpk_flatpak_installer_finalize;
}

static void
gpk_flatpak_installer_init (GpkFlatpakInstaller *installer)
{
	GpkFlatpakInstallerPrivate *priv;

	priv = gpk_flatpak_installer_get_instance_private (installer);

	/* Creates a new FlatpakInstallation for system-wide installation.*/
	priv->installation = flatpak_installation_new_system (NULL, NULL);

	/* Create a new FlatpakRef */
	priv->flatpakref = gpk_flatpak_ref_new ();
}

static void
gpk_flatpak_installer_finalize (GObject *object)
{
	GpkFlatpakInstallerPrivate *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GPK_IS_FLATPAK_INSTALLER (object));

	priv = gpk_flatpak_installer_get_instance_private (GPK_FLATPAK_INSTALLER (object));

	if (priv->installation)
		g_object_unref (priv->installation);
	if (priv->flatpakref)
		g_object_unref (priv->flatpakref);

	G_OBJECT_CLASS (gpk_flatpak_installer_parent_class)->finalize (object);
}

GpkFlatpakInstaller *
gpk_flatpak_installer_new (void)
{
	GpkFlatpakInstaller *flatpakref = g_object_new (GPK_TYPE_FLATPAK_INSTALLER, NULL);
	return GPK_FLATPAK_INSTALLER (flatpakref);
}
