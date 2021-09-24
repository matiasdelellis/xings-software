/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
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

#ifndef __GPK_UPDATES_CHECKER_H
#define __GPK_UPDATES_CHECKER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GPK_TYPE_UPDATES_CHECKER (gpk_updates_checker_get_type ())

G_DECLARE_FINAL_TYPE (GpkUpdatesChecker, gpk_updates_checker, GPK, UPDATES_CHECKER, GObject)

guint              gpk_updates_checker_get_important_updates_count (GpkUpdatesChecker *checker);

guint              gpk_updates_checker_get_updates_count           (GpkUpdatesChecker *checker);

gchar            **gpk_updates_checker_get_update_packages_ids     (GpkUpdatesChecker *checker);

void               gpk_updates_checker_check_for_updates           (GpkUpdatesChecker *checker);

GpkUpdatesChecker *gpk_updates_checker_new (void);

G_END_DECLS

#endif /* __GPK_UPDATES_CHECKER_H */
