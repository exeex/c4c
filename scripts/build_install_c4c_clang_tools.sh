#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${ROOT_DIR}/ref/c4c-clang-tools"
BUILD_DIR="${ROOT_DIR}/build/c4c-clang-tools"
INSTALL_PREFIX="${1:-${HOME}/.local}"

cmake -S "${SRC_DIR}" -B "${BUILD_DIR}"
cmake --build "${BUILD_DIR}" -j"${C4C_CLANG_TOOLS_JOBS:-2}"
cmake --install "${BUILD_DIR}" --prefix "${INSTALL_PREFIX}"

echo "installed c4c clang tools to ${INSTALL_PREFIX}/bin"
