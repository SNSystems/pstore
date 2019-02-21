#!/usr/bin/env bash
#===- llvm/tools/pstore/utils/docker/scripts/checkout_pstore.sh -----------===//
#
# Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===-----------------------------------------------------------------------===//

set -e

function show_usage() {
  cat << EOF
Usage: checkout_pstore.sh [options]

Checkout git sources into /tmp/clang-build/src. Used inside a docker container.

Available options:
  -h|--help           show this help message
  -b|--branch         svn branch to checkout, i.e. 'trunk',
                      'branches/release_40'
                      (default: 'trunk')
EOF
}

REMOTE_BRANCH=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    -b|--branch)
      shift
      REMOTE_BRANCH="$1"
      shift
      ;;
    -h|--help)
      show_usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1"
      exit 1
  esac
done

if [ "$REMOTE_BRANCH" == "" ]; then
  REMOTE_BRANCH="master"
fi

PSTORE_BUILD_DIR=/tmp/pstore-build

# Get the sources from git.
echo "Checking out sources from git"
CHECKOUT_DIR="$PSTORE_BUILD_DIR/src/"
mkdir -p $CHECKOUT_DIR

REMOTE_REPO="https://github.com/SNSystems"
LLVM_PROJECT="llvm-prepo.git"
CLANG_PROJECT="clang-prepo.git"
PSTORE_PROJECT="pstore.git"

function clone_project() {
  local PROJ="$1"
  local DEST="$2"
  local BRANCH=$REMOTE_BRANCH

  # Check if remote branch exists.
  set +e
  git ls-remote --heads --exit-code $REMOTE_REPO/$PROJ $BRANCH
  if [ "$?" == "2" ]; then
    echo "Branch $BRANCH does not exist. Using 'master'."
    BRANCH="master"
  fi
  set -e
  echo "Checking out $BRANCH/$1 to $DEST"
  git clone --branch $BRANCH $REMOTE_REPO/$PROJ $DEST
}

cd $CHECKOUT_DIR
clone_project $LLVM_PROJECT llvm

cd $CHECKOUT_DIR/llvm/tools
clone_project $CLANG_PROJECT clang
clone_project $PSTORE_PROJECT pstore
cd -

echo "Done"
