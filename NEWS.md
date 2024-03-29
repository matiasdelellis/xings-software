# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Version 0.3.94] - 2024-02-20
### Added
- Show an action menu to show third party app store.

### Removed
- Remove the Software history application again.

### Fixed
- Fix installed package detection in Debian Bookworm
- Lot fixes and improved code

## [Version 0.3.92] - 2023-04-11
### Added
- Use radio menu items to show that is searching.
- Improves the fetching of the icons from components.
- Remove old code and modernize some code.
- Update repositories on first run. Issue #9.
- Use appstream icons also for cached icons.
- Search by main categories. See ximion/appstream#456 for context.
- Initial version of Xings Third Parties to install apps with flatpakrefs.
- Add support for special categories, as "Editor's suggestions" or "New applications".

## [Version 0.3.91] - 2022-03-15
### Added
 - Yet another modernization of the main interface of software.
 - Show app icon in software application.
 - Notify when finish installing a package with name or local file. Issue #2
 - Never check for updates on low battery. Issue #7
 - Don't download updates when battery is discharging and the updates. Issue #7

### Fixed
- Fix some memory leaks.
- Fix build without systemd, and small clean.

## [Version 0.3.90] - 2022-02-15
### Added
 - Integration with Appstream, to show the most relevant information for users,
   and organize applications.
 - Allows browsing applications by categories, and searching for applications,
   but also distribution packages.
 - Add a button to launch installed apps instead annoying dialogue.
 - Add the link to reporting, translations and donations.

### Fixed
 - Fix dont build on EPEL 7 due "for" loop initial declarations. Fix issue #4
 - No need to bother the user for not having the details of the update.
 - Don't use PK_FILTER_ENUM_NEWEST filter when getting updates.
 - No need to bother the user for not having the details of the update.

### Regressions
Although I do remove them with conviction, recognize that they are regressions,
and that they are probably annoying for some users.
 - Remove search for files feature.
 - Remove search by detail, and only search by name.
 - Remove the option to remove or install multiple packages.
 - Remove the option to show only newest and native packages.
 - Remove dependencies, required and package files dialogs.

## [Version 0.3.5] - 2021-10-22
### Added
 - Hoping for a better future, we called the project Xings Software.
 - First attempt to modernize the software installation application.
 - Add pending button to review changes before applying them.
 - Show update downloads in history dialog.
 - Add --enable-offline-updates to build configuration disabled by default.

### Fixed
 - Modernize appstream metadata. Issue #1
 - Add man page xings-software-service. Issue #1
 - Add some keywords to desktops file, and descriptions improvements. Issue #1
 - Adhered to keepachangelog/semantic versioning and fix date of last release.
 - Fix icon of history and preferences.
 - Don't search on paste. It is quite confusing.

## [Version 0.3.2] - 2021-10-07
### Added
 - Add a button to check again when it says there are no updates.
 - Add support to control session with xfce4 session manager.

### Fixed
 - Remove some unused variable warnings.
 - Dont select the title headers.
 - Take into account the download size to know if the update is downloaded.

## [0.3.1] - 2021-09-30
### Added
 - Add and friendly empty page instead a annoying modal dialog when updating.
 - Add the option to check for updates even on mobile connections.

### Fixed
 - Remove unused icons with conflict with gnome-packagekit.
 - Center the windows and reduce its size by default.
 - Fix xfce4-settings and mate-control-center integration.
 - Remove the check for updates from Package installer.
 - Remove some unused code.
 - Fix some ghost package in the listing when updating with some backends.

## [0.3.0] - 2021-09-28
### Added
 - Add a user service that update the repository and checks for updates.
 - It can automatically download updates to install when shutdown the computer.
 - Show "Software Settings" in xfce4-settings-manager and mate-control-center.

### Fixed
 - Fix run and select apps dialogs.
 - Fix systemd logind integration to restart. Although it is not currently used.
 - Fix use without initialization that caused a segmentation fault.
 - Rename packageKit service to avoid conflict with gnome-software.

## [0.2.0] - 2021-06-18
### Added
 - Revert "Remove the gpk-install-package-name binary", and now it's called
   "xings-install-package-name".
 - Revert "Do not use PkDesktop" and port run the new installed package to pure
   PackageKit.
 - Remove animations from dialogs.

### Fixed
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

### Backports
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

## [0.1.0] - 2021-06-16
### Added
 - The first public release as xings-packagekit.

## Previous versions

This project is based on gnome-packagekit v3.14. Observe their NEWS if you are
interested:
* https://gitlab.gnome.org/GNOME/gnome-packagekit/-/blob/gnome-3-14/NEWS
