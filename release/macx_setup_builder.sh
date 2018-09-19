#!/bin/sh

# Note: The structure of this package depends on the -rpath,./lib to be set at compile/link time.

version="1.2.0"
arch=`uname -m`

if [ "${arch}" = "x86_64" ]; then
    arch="64bit"
else
    arch="32bit"
fi

if [ -f Verium.app/Contents/MacOS/Verium ] && [ -f verium.conf ] ; then
    echo "Building Verium_${version}_${arch}.pkg ...\n"
    cp verium.conf Verium.app/Contents/MacOS/

    # Remove the old archive
    if [ -f Verium_${version}_${arch}.pkg ]; then
        rm -f Verium_${version}_${arch}.pkg
    fi

    # Deploy the app, create the plist, then build the package.
    macdeployqt ./Verium.app -always-overwrite
    pkgbuild --analyze --root ./Verium.app share/qt/Verium.plist
    pkgbuild --root ./Verium.app --component-plist share/qt/Verium.plist --identifier org.verium.Verium --install-location /Applications/Verium.app Verium_${version}_${arch}.pkg
    echo "Package created in: $PWD/Verium_${version}_${arch}.pkg\n"
else
    echo "Error: Missing files!\n"
    echo "Run this script from the folder containing Verium-Qt.app, verium.conf and README.\n"
fi

