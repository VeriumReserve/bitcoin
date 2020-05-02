#!/usr/bin/env bash
#
# Copyright (c) 2018-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

BITCOIN_CONFIG_ALL="--disable-dependency-tracking --prefix=$DEPENDS_DIR/$HOST --bindir=$BASE_OUTDIR/bin --libdir=$BASE_OUTDIR/lib"
if [ -z "$NO_DEPENDS" ]; then
  DOCKER_EXEC ccache --max-size=$CCACHE_SIZE
fi

if [ "$INSTALL_DB4" = "true" ]; then
  BEGIN_FOLD db4
  DOCKER_EXEC ./contrib/install_db4.sh $DEPENDS_DIR > /dev/null # only error log
  BITCOIN_CONFIG_ALL+=" BDB_LIBS=\"-L${DEPENDS_DIR}/db4/lib -ldb_cxx-4.8\" BDB_CFLAGS=\"-I${DEPENDS_DIR}/db4/include\""
  END_FOLD
fi

BEGIN_FOLD autogen
if [ -n "$CONFIG_SHELL" ]; then
  DOCKER_EXEC "$CONFIG_SHELL" -c "./autogen.sh" 
else
  DOCKER_EXEC ./autogen.sh
fi
END_FOLD

DOCKER_EXEC mkdir -p build
export P_CI_DIR="$P_CI_DIR/build"

BEGIN_FOLD configure
DOCKER_EXEC ../configure --cache-file=config.cache $BITCOIN_CONFIG_ALL $BITCOIN_CONFIG || ( (DOCKER_EXEC cat config.log) && false)
END_FOLD

BEGIN_FOLD distdir
# Create folder on host and docker, so that `cd` works
mkdir -p "verium-$HOST"
DOCKER_EXEC make distdir VERSION=$HOST
END_FOLD

export P_CI_DIR="$P_CI_DIR/verium-$HOST"

BEGIN_FOLD configure
DOCKER_EXEC ./configure --cache-file=../config.cache $BITCOIN_CONFIG_ALL $BITCOIN_CONFIG || ( (DOCKER_EXEC cat config.log) && false)
END_FOLD

set -o errtrace
trap 'DOCKER_EXEC "cat ${BASE_SCRATCH_DIR}/sanitizer-output/* 2> /dev/null"' ERR

BEGIN_FOLD build
DOCKER_EXEC make $MAKEJOBS $GOAL || ( echo "Build failure. Verbose build follows." && DOCKER_EXEC make $GOAL V=1 ; false )
END_FOLD
