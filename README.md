# Xings Software

The **Xings Software** project is a set of clients for [PackageKit](https://www.freedesktop.org/software/PackageKit/), designed to
integrate with GTK desktops. Of course, in terms of design and integration, we
mean that we use this toolkit, hoping that it works on any desktop beyond Xfce
or Mate.

This is a fork of [gnome-packagekit](https://gitlab.gnome.org/GNOME/gnome-packagekit) version 3.14.3 that we consider is the last
one with proper integration with other desktops.

## Main Software
* **Xings Software:** Add or remove software installed on the system.
* **Xings Software Update:** Update software installed on the system.
* **Xings Software Preferences:** Change software update preferences and
  enable or disable software sources.
* **Xings Software History:** View past package management tasks.

## User Service
* **xings-updates-service:** It allows to synchronize the repositories
  regularly, searching and even downloading the updates.
## D-Bus Service
* **org.xings.PackageKit.service:** Implement the `org.freedesktop.PackageKit`
  D-Bus service, allowing other applications to install software

## Screenshots

![Xings-PackageKit](data/appdata/ss-application-details.png)

## Acknowledgments

Many thanks to Richard Hughes and all subsequent maintainers of the entire
PackageKit ecocystem.
