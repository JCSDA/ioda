#!/bin/sh

set -ex

SRC_DIR=${1:-"./"}
PYCODESTYLE_CFG_PATH=${2:-"./"}

cd $SRC_DIR

pycodestyle -v --config="$PYCODESTYLE_CFG_PATH/.pycodestyle" --filename=*.py,*.py.in .
