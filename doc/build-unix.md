# UNIX BUILD NOTES

Some notes on how to build Verium Core in Unix.

(for OpenBSD specific instructions, see [build-openbsd.md](build-openbsd.md))

## Table of Contents
----------------------------------------------
1. [Note](#note)
2. [How to Build](#how-to-build)
3. [Dependencies](#dependencies)
4. [Memory Requirements](#memory-requirements)
5. [Build Instructions](#build-instructions)
    1. [Ubuntu and Debian](#ubuntu-and-debian)
    2. [Fedora](#fedora)
    3. [Arch Linux](#arch-linux)
    4. [FreeBSD](#freebsd)
6. [Build Notes](#build-notes)
7. [Miniupnpc](#miniupnpc)
8. [Berkeley DB](#berkeley-db)
9. [Boost](#boost)
10. [Security](#security)
11. [Disable-wallet mode](#disable-wallet-mode)
12. [Additional Configure Flags](#additional-configure-flags)
13. [ARM Cross-compilation](#arm-cross-compilation)


## Note
----------------------------------------------

Always use absolute paths to configure and compile verium and the dependencies,
for example, when specifying the path of the dependency:

	../dist/configure --enable-cxx --disable-shared --with-pic --prefix=$BDB_PREFIX

Here BDB_PREFIX must be an absolute path - it is defined using $(pwd) which ensures
the usage of the absolute path.

## How To Build
----------------------------------------------

```bash
./autogen.sh
./configure
make
make install # optional
```

This will build verium-qt as well if the dependencies are met.

## Dependencies
----------------------------------------------

These dependencies are required:

 Library     | Purpose          | Description
 ------------|------------------|----------------------
 libssl      | Crypto           | Random Number Generation, Elliptic Curve Cryptography
 libboost    | Utility          | Library for threading, data structures, etc
 libevent    | Networking       | OS independent asynchronous networking
 libcurl     | Bootstrap        | Library for download bootstrap
 zlib        |                  | Library for compression
 minizip     |                  | Library for compression

Optional dependencies:

 Library     | Purpose          | Description
 ------------|------------------|----------------------
 miniupnpc   | UPnP Support     | Firewall-jumping support
 libdb4.8    | Berkeley DB      | Wallet storage (only needed when wallet enabled)
 qt          | GUI              | GUI toolkit (only needed when GUI enabled)
 protobuf    | Payments in GUI  | Data interchange format used for payment protocol (only needed when GUI enabled)
 libqrencode | QR codes in GUI  | Optional for generating QR codes (only needed when GUI enabled)
 univalue    | Utility          | JSON parsing and encoding (bundled version will be used unless --with-system-univalue passed to configure)
 libzmq3     | ZMQ notification | Optional, allows generating ZMQ notifications (requires ZMQ version >= 4.x)

For the versions used, see [dependencies.md](dependencies.md)

## Memory Requirements
----------------------------------------------

C++ compilers are memory-hungry. It is recommended to have at least 1.5 GB of
memory available when compiling Verium Core. On systems with less, gcc can be
tuned to conserve memory with additional CXXFLAGS:


    ./configure CXXFLAGS="--param ggc-min-expand=1 --param ggc-min-heapsize=32768"

## Build Instructions

### Ubuntu and Debian
----------------------------------------------
Build requirements:

    sudo apt-get install build-essential libtool autotools-dev automake pkg-config libssl-dev libevent-dev bsdmainutils python3 libcurl4-openssl-dev zlib1g-dev libminizip-dev

Options when installing required Boost library files:

1. On at least Ubuntu 14.04+ and Debian 7+ there are generic names for the
individual boost development packages, so the following can be used to only
install necessary parts of boost:

        sudo apt-get install libboost-system-dev libboost-filesystem-dev libboost-chrono-dev libboost-program-options-dev libboost-test-dev libboost-thread-dev

2. If that doesn't work, you can install all boost development packages with:

        sudo apt-get install libboost-all-dev

BerkeleyDB is required for the wallet.

Ubuntu and Debian have their own libdb-dev and libdb++-dev packages, but these will install
BerkeleyDB 5.1 or later, which break binary wallet compatibility with the distributed executables which
are based on BerkeleyDB 4.8. If you do not care about wallet compatibility,
pass `--with-incompatible-bdb` to configure.

To build and use BerkeleyDB 4.8 please referer to the [Berkeley DB](#berkeley-db)

See the section "Disable-wallet mode" to build Verium Core without wallet.

Optional (see --with-miniupnpc and --enable-upnp-default):

    sudo apt-get install libminiupnpc-dev

ZMQ dependencies (provides ZMQ API 4.x):

    sudo apt-get install libzmq3-dev


#### GUI
If you want to build Verium-Qt, make sure that the required packages for Qt development
are installed. Either Qt 5 or Qt 4 are necessary to build the GUI.
If both Qt 4 and Qt 5 are installed, Qt 5 will be used. Pass `--with-gui=qt4` to configure to choose Qt4.
To build without GUI pass `--without-gui`.

To build with Qt 5 (recommended) you need the following:

    sudo apt-get install libqt5gui5 libqt5core5a libqt5dbus5 qttools5-dev qttools5-dev-tools libprotobuf-dev protobuf-compiler

Alternatively, to build with Qt 4 you need the following:

    sudo apt-get install libqt4-dev libprotobuf-dev protobuf-compiler

libqrencode (optional) can be installed with:

    sudo apt-get install libqrencode-dev

Once these are installed, they will be found by configure and a verium-qt executable will be
built by default.

#### Quick Build
Bash script to quickly download and build verium with GUI

```sh
sudo apt-get install -y git build-essential libtool autotools-dev automake pkg-config libssl-dev libevent-dev bsdmainutils python3 libcurl4-openssl-dev libboost-system-dev libboost-filesystem-dev libboost-chrono-dev libboost-program-options-dev libboost-test-dev libboost-thread-dev libminizip-dev zlib1g-dev libqt5gui5 libqt5core5a libqt5dbus5 qttools5-dev qttools5-dev-tools libprotobuf-dev protobuf-compiler libqrencode-dev
git clone https://github.com/VeriumReserve/verium.git ~/verium
cd ~/verium
./contrib/install_db4.sh ~/verium
export BDB_PREFIX="${HOME}/verium/db4"
./autogen.sh
./configure BDB_LIBS="-L${BDB_PREFIX}/lib -ldb_cxx-4.8" BDB_CFLAGS="-I${BDB_PREFIX}/include"
make
```

### Fedora
----------------------------------------------
Build requirements:

    sudo dnf install gcc-c++ libtool make autoconf automake openssl-devel libevent-devel boost-devel libdb4-devel libdb4-cxx-devel python3 libcurl-devel minizip-devel

Optional:

    sudo dnf install miniupnpc-devel

To build with Qt 5 (recommended) you need the following:

    sudo dnf install qt5-qttools-devel qt5-qtbase-devel protobuf-devel

libqrencode (optional) can be installed with:

    sudo dnf install qrencode-devel

To build and use BerkeleyDB 4.8 please referer to the [Berkeley DB](#berkeley-db)


Bash script to quickly download and build verium with GUI

#### Quick Build
Bash script to quickly download and build verium with GUI

```sh
sudo dnf install git gcc-c++ libtool make autoconf automake openssl-devel libevent-devel boost-devel libdb4-devel libdb4-cxx-devel python3 libcurl-devel minizip-devel qt5-qttools-devel qt5-qtbase-devel protobuf-devel qrencode-devel
git clone https://github.com/VeriumReserve/verium.git ~/verium
cd ~/verium
./contrib/install_db4.sh ~/verium
export BDB_PREFIX="${HOME}/verium/db4"
./autogen.sh
./configure BDB_LIBS="-L${BDB_PREFIX}/lib -ldb_cxx-4.8" BDB_CFLAGS="-I${BDB_PREFIX}/include"
make
```

#### Arch Linux
----------------------------------------------
Build requirements:

    sudo pacman -S base-devel boost libevent python minizip

Optional:

    sudo pacman -S install miniupnpc

To build with Qt 5 (recommended) you need the following:

    sudo dnf install qt5-qttools qt5-qtbase protobuf

libqrencode (optional) can be installed with:

    sudo dnf install qrencode

To build and use BerkeleyDB 4.8 please referer to the [Berkeley DB](#berkeley-db)

#### Quick Build
Bash script to quickly download and build verium with GUI

```sh
sudo pacman -S git base-devel boost libevent python minizip qt5-qttools qt5-qtbase protobuf qrencode
git clone https://github.com/VeriumReserve/verium.git ~/verium
cd ~/verium
./contrib/install_db4.sh ~/verium
export BDB_PREFIX="${HOME}/verium/db4"
./autogen.sh
./configure BDB_LIBS="-L${BDB_PREFIX}/lib -ldb_cxx-4.8" BDB_CFLAGS="-I${BDB_PREFIX}/include"
make
```

#### FreeBSD
----------------------------------------------
Clang is installed by default as `cc` compiler, this makes it easier to get
started than on [OpenBSD](build-openbsd.md). Installing dependencies:

    pkg install autoconf automake libtool pkgconf boost-libs openssl libevent gmake curl minizip

You need to use GNU make (`gmake`) instead of `make`.
(`libressl` instead of `openssl` will also work)

For the wallet (optional):

    ./contrib/install_db4.sh `pwd`
    export BDB_PREFIX $PWD/db4

Then build using:

    ./autogen.sh
    ./configure BDB_LIBS="-L${BDB_PREFIX}/lib -ldb_cxx-4.8" BDB_CFLAGS="-I${BDB_PREFIX}/include" LDFLAGS="-L/usr/local/lib/" CPPFLAGS="-I/usr/local/include/" MAKE="gmake"
    gmake

*Note on debugging*: The version of `gdb` installed by default is [ancient and considered harmful](https://wiki.freebsd.org/GdbRetirement).
It is not suitable for debugging a multi-threaded C++ program, not even for getting backtraces. Please install the package `gdb` and
use the versioned gdb command e.g. `gdb7111`.

#### Quick Build
Bash script to quickly download and build verium with GUI

```sh
sudo pkg install git autoconf automake libtool pkgconf boost-libs openssl libevent gmake curl minizip
git clone https://github.com/VeriumReserve/verium.git ~/verium
cd ~/verium
./contrib/install_db4.sh ~/verium
export BDB_PREFIX="${HOME}/verium/db4"
./autogen.sh
./configure BDB_LIBS="-L${BDB_PREFIX}/lib -ldb_cxx-4.8" BDB_CFLAGS="-I${BDB_PREFIX}/include" LDFLAGS="-L/usr/local/lib/" CPPFLAGS="-I/usr/local/include/" MAKE="gmake"
gmake
```


## Build Notes
----------------------------------------------
The release is built with GCC and then "strip veriumd" to strip the debug
symbols, which reduces the executable size by about 90%.


## Miniupnpc
----------------------------------------------

[miniupnpc](http://miniupnp.free.fr/) may be used for UPnP port mapping.  It can be downloaded from [here](
http://miniupnp.tuxfamily.org/files/).  UPnP support is compiled in and
turned off by default.  See the configure options for upnp behavior desired:

	--without-miniupnpc      No UPnP support miniupnp not required
	--disable-upnp-default   (the default) UPnP support turned off by default at runtime
	--enable-upnp-default    UPnP support turned on by default at runtime


## Berkeley DB
----------------------------------------------
It is recommended to use Berkeley DB 4.8. If you have to build it yourself,
you can use [the installation script included in contrib/](/contrib/install_db4.sh)
like so

```shell
./contrib/install_db4.sh `pwd`
```

from the root of the repository.

**Note**: You only need Berkeley DB if the wallet is enabled (see the section *Disable-Wallet mode* below).

## Boost
-----
If you need to build Boost yourself:

	sudo su
	./bootstrap.sh
	./bjam install


## Security
--------
To help make your verium installation more secure by making certain attacks impossible to
exploit even if a vulnerability is found, binaries are hardened by default.
This can be disabled with:

Hardening Flags:

	./configure --enable-hardening
	./configure --disable-hardening


Hardening enables the following features:

* Position Independent Executable
    Build position independent code to take advantage of Address Space Layout Randomization
    offered by some kernels. Attackers who can cause execution of code at an arbitrary memory
    location are thwarted if they don't know where anything useful is located.
    The stack and heap are randomly located by default but this allows the code section to be
    randomly located as well.

    On an AMD64 processor where a library was not compiled with -fPIC, this will cause an error
    such as: "relocation R_X86_64_32 against `......' can not be used when making a shared object;"

    To test that you have built PIE executable, install scanelf, part of paxutils, and use:

    	scanelf -e ./verium

    The output should contain:

     TYPE
    ET_DYN

* Non-executable Stack
    If the stack is executable then trivial stack based buffer overflow exploits are possible if
    vulnerable buffers are found. By default, verium should be built with a non-executable stack
    but if one of the libraries it uses asks for an executable stack or someone makes a mistake
    and uses a compiler extension which requires an executable stack, it will silently build an
    executable without the non-executable stack protection.

    To verify that the stack is non-executable after compiling use:
    `scanelf -e ./verium`

    the output should contain:
	STK/REL/PTL
	RW- R-- RW-

    The STK RW- means that the stack is readable and writeable but not executable.

## Disable-wallet mode
--------------------
When the intention is to run only a P2P node without a wallet, verium may be compiled in
disable-wallet mode with:

    ./configure --disable-wallet

In this case there is no dependency on Berkeley DB 4.8.

Mining is also possible in disable-wallet mode, but only using the `getblocktemplate` RPC
call not `getwork`.

## Additional Configure Flags
----------------------------------------------
A list of additional configure flags can be displayed with:

    ./configure --help

## ARM Cross-compilation
----------------------------------------------
These steps can be performed on, for example, an Ubuntu VM. The depends system
will also work on other Linux distributions, however the commands for
installing the toolchain will be different.

Make sure you install the build requirements mentioned above.
Then, install the toolchain and curl:

    sudo apt-get install g++-arm-linux-gnueabihf curl

To build executables for ARM:

    cd depends
    make HOST=arm-linux-gnueabihf NO_QT=1
    cd ..
    ./configure --prefix=$PWD/depends/arm-linux-gnueabihf --enable-glibc-back-compat --enable-reduce-exports LDFLAGS=-static-libstdc++
    make


For further documentation on the depends system see [README.md](../depends/README.md) in the depends directory.
