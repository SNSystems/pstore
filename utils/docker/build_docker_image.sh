#!/bin/bash
#===- llvm/tools/pstore/utils/docker/build_docker_image.sh ----------------===//
#
# Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===-----------------------------------------------------------------------===//
set -e

IMAGE_SOURCE="pstore"
DOCKER_REPOSITORY=""
DOCKER_TAG=""
BUILDSCRIPT_ARGS=""
CHECKOUT_ARGS=""

function show_usage() {
  cat << EOF
Usage: build_docker_image.sh [options] [-- [cmake_args]...]

Available options:
  General:
    -h|--help               show this help message
  Docker-specific:
    -t|--docker-tag         docker tag for the image
    -d|--docker-repository  docker repository for the image
  Checkout arguments:
    -b|--branch             svn branch to checkout, i.e. 'trunk',
                            'branches/release_40'
                            (default: 'trunk')
  Build-specific:
    -i|--install-target     name of a cmake install target to build and
                            include in the resulting archive. Can be
                            specified multiple times.

Required options: --docker-repository, at least one --install-target.

All options after '--' are passed to CMake invocation.

For example, running:
  build_docker_image.sh \
    --branch master --docker-repository pstore --docker-tag latest \
    --install-target install-clang --install-target install-pstore \
    -- -DCMAKE_BUILD_TYPE=Release

will produce a docker image:
    pstore:latest - a small image with preinstalled release versions of
                    clang and pstore, build from the 'master' branch.
EOF
}

SEEN_INSTALL_TARGET=0
SEEN_CMAKE_ARGS=0
while [[ $# -gt 0 ]]; do
  case "$1" in
    -h|--help)
      show_usage
      exit 0
      ;;
    -d|--docker-repository)
      shift
      DOCKER_REPOSITORY="$1"
      shift
      ;;
    -t|--docker-tag)
      shift
      DOCKER_TAG="$1"
      shift
      ;;
    -b|--branch)
      CHECKOUT_ARGS="$CHECKOUT_ARGS $1 $2"
      shift 2
      ;;
    -i|--install-target)
      SEEN_INSTALL_TARGET=1
      BUILDSCRIPT_ARGS="$BUILDSCRIPT_ARGS $1 $2"
      shift 2
      ;;
    --)
      shift
      BUILDSCRIPT_ARGS="$BUILDSCRIPT_ARGS -- $*"
      SEEN_CMAKE_ARGS=1
      shift $#
      ;;
    *)
      echo "Unknown argument $1"
      exit 1
      ;;
  esac
done

command -v docker >/dev/null ||
  {
    echo "Docker binary cannot be found. Please install Docker to use this script."
    exit 1
  }

if [ "$DOCKER_REPOSITORY" == "" ]; then
  echo "Required argument missing: --docker-repository"
  exit 1
fi

if [ $SEEN_INSTALL_TARGET -eq 0 ]; then
  echo "Please provide at least one --install-target"
  exit 1
fi

SOURCE_DIR=$(dirname $0)
if [ ! -d "$SOURCE_DIR/$IMAGE_SOURCE" ]; then
  echo "No sources for '$IMAGE_SOURCE' were found in $SOURCE_DIR"
  exit 1
fi

BUILD_DIR=$(mktemp -d)
trap "rm -rf $BUILD_DIR" EXIT
echo "Using a temporary directory for the build: $BUILD_DIR"

cp -r "$SOURCE_DIR/$IMAGE_SOURCE" "$BUILD_DIR/$IMAGE_SOURCE"
cp -r "$SOURCE_DIR/scripts" "$BUILD_DIR/scripts"

if [ "$DOCKER_TAG" != "" ]; then
  DOCKER_TAG=":$DOCKER_TAG"
fi

echo "BUILDSCRIPT_ARGS:  '$BUILDSCRIPT_ARGS'"
echo "BUILD_DIR:         '$BUILD_DIR'"
echo "CHECKOUT_ARGS:     '$CHECKOUT_ARGS'"
echo "DOCKER_REPOSITORY: '$DOCKER_REPOSITORY'"
echo "DOCKER_TAG:        '$DOCKER_TAG'"
echo "IMAGE_SOURCE:      '$IMAGE_SOURCE'"

echo "Building $DOCKER_REPOSITORY$DOCKER_TAG from $IMAGE_SOURCE"
docker build --tag "$DOCKER_REPOSITORY$DOCKER_TAG" \
  --build-arg "checkout_args=$CHECKOUT_ARGS" \
  --build-arg "buildscript_args=$BUILDSCRIPT_ARGS" \
  --file "$BUILD_DIR/$IMAGE_SOURCE/Dockerfile" \
  --rm \
  "$BUILD_DIR"
echo "Done"
