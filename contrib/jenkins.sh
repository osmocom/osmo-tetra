#!/bin/sh -ex

rm -rf deps/install
mkdir deps || true
cd deps
osmo-deps.sh libosmocore

cd libosmocore
autoreconf --install --force
./configure --prefix=$PWD/../install
$MAKE $PARALLEL_MAKE install

cd ../../src
make clean || true
PKG_CONFIG_PATH=$PWD/../deps/install/lib/pkgconfig $MAKE
