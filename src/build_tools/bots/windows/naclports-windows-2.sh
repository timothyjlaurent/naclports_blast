#!/bin/bash
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This script builds the packages for Native Client that are designated to
# the bot named windows-<libc>-2.

source ../bot_common.sh

SCRIPT_DIR="$(cd $(dirname $0) && pwd)"
cd ${SCRIPT_DIR}/../../..
make clean

# nethack/dosbox also builds nacl-mounts
BuildPackage nethack
BuildPackage dosbox
BuildPackage snes9x
# flac also builds ogg
# ogg is built by the bot windows-ports-0, but the package is re-built here as
# part of the load balancing
BuildPackage flac
BuildPackage cfitsio
BuildPackage speex

echo "@@@BUILD_STEP ${NACL_ARCH} Summary@@@"
if [[ $RESULT != 0 ]] ; then
  echo "@@@STEP_FAILURE@@@"
fi

echo -e "$MESSAGES"

exit $RESULT
