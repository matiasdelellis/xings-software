Version 0.3.0
~~~~~~~~~~~~~
Released: 2021-09-28

* New Features:
 - Add a user service that update the repository and checks for updates.
 - It can automatically download updates to install when shutdown the computer.
 - Show "Software Settings" in xfce4-settings-manager and mate-control-center.

Bug fixes:
 - Fix run and select apps dialogs.
 - Fix systemd logind integration to restart. Although it is not currently used.
 - Fix use without initialization that caused a segmentation fault.
 - Rename packageKit service to avoid conflict with gnome-software.

Version 0.2.0
~~~~~~~~~~~~~
Released: 2021-06-18

* New Features:
 - Revert "Remove the gpk-install-package-name binary", and now it's called
   "xings-install-package-name".
 - Revert "Do not use PkDesktop" and port run the new installed package to pure
   PackageKit.
 - Remove animations from dialogs.

Bug fixes:
 - Application: update application ID to match .desktop file.
 - Fix an use of deprecated g_time_val_from_iso8601() function.
 - Update .gitignore files to new brands
 - Fix last use of deprecated g_time_val_from_iso8601() function.
 - Fix usage of deprecated gtk_menu_popup() function.
 - Use G_DECLARE_FINAL_TYPE on GpkX11 and remove deprectated functions.
 - Use G_DECLARE_FINAL_TYPE on G_DECLARE_DERIVABLE_TYPE all GObjects.
 - Fix help on install local-package and package-name.
 - Prettyify the build output.
 - Stop using gtk_show_uri() deprecated.
 - Fix last usage of deprecated functions.
 - Fix make distcheck.

* Backports:
 - Consider available packages as updatable.
 - Use gtk_text_buffer_insert_markup() from GTK+.
 - Remove the markdown parsing module.
 - trivial: Inline a simple string test.
 - Depend on a non-obsolete PackageKit version.
 - trivial: Remove some unused functionality.
 - Use more GNOME_COMPILE_WARNINGS.
 - Don't use the deprecated GNOME_COMPILE_WARNINGS.
 - trivial: Use AX_APPEND_COMPILE_FLAGS directly.
 - trivial: Use the standard PKGDATADIR.
 - Always show the correct data for the source label.
 - trivial: Ensure gpk-application gets the warning flags set.
 - Use G_DECLARE_FINAL_TYPE.
 - Add new status "Running hooks".
 - trivial: Add the missing translated versions of the new PK enums.
 - trivial: Don't use deprecated GTK API.
 - Ensure to escape package and vendor name.
 - Update the UI files to GTK 3.18 and adhere to the GNOME 3 UI guidelines.

Version 0.1.0
~~~~~~~~~~~~~
Released: 2021-06-16

* New Features:
 - The first public release as xings-packagekit.

Previous versions
~~~~~~~~~~~~~~~~~
This project is based on gnome-packagekit v3.14. Observe their NEWS if you are
interested:
* https://gitlab.gnome.org/GNOME/gnome-packagekit/-/blob/gnome-3-14/NEWS
