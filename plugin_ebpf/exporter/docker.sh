#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

# This script holds docker related functions.

REPO_ROOT=$(dirname "${BASH_SOURCE[0]}")/../..
REGISTRY=${REGISTRY:-"docker.io/littlebull"}
VERSION=${VERSION:="unknown"}

function build_images() {
  local target="$1"
  docker build --rm -t ${REGISTRY}/${target}:${VERSION} -f ./Dockerfile ${REPO_ROOT}
}

build_images $@