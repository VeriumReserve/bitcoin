#!/usr/bin/env bash
#
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native
export PACKAGES="python3-zmq"
export GOAL="install"
export BITCOIN_CONFIG="--enable-glibc-back-compat --enable-reduce-exports --with-gui=no"
export INSTALL_DB4="true"