
Debian
====================
This directory contains files used to package veriumd/verium-qt
for Debian-based Linux systems. If you compile veriumd/verium-qt yourself, there are some useful files here.

## verium: URI support ##


verium-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install verium-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your verium-qt binary to `/usr/bin`
and the `../../share/pixmaps/verium.png` to `/usr/share/pixmaps`

verium-qt.protocol (KDE)

