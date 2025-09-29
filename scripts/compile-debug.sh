#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}/.."
BUILD_DIR="${AUTOGITPULL_BUILD_DIR:-${ROOT_DIR}/build-debug}"
BUILD_TYPE=Debug

CMAKE_ARGS=(
    "-DCMAKE_BUILD_TYPE=${BUILD_TYPE}"
    "-DCMAKE_CXX_FLAGS_DEBUG=-fsanitize=address"
    "-DCMAKE_EXE_LINKER_FLAGS_DEBUG=-fsanitize=address"
)

if [[ "${AUTOGITPULL_GENERATOR:-}" != "" ]]; then
    CMAKE_ARGS+=("-G" "${AUTOGITPULL_GENERATOR}")
fi

if [[ "$(uname -s)" == "Darwin" ]]; then
    ARCHES="${AUTOGITPULL_OSX_ARCHS:-}"
    if [[ -z "${ARCHES}" ]]; then
        ARCHES=$(uname -m)
    fi
    CMAKE_ARGS+=("-DCMAKE_OSX_ARCHITECTURES=${ARCHES}")
fi

if [[ -n "${AUTOGITPULL_CMAKE_EXTRA_ARGS:-}" ]]; then
    # shellcheck disable=SC2206
    EXTRA_ARGS=( ${AUTOGITPULL_CMAKE_EXTRA_ARGS} )
    CMAKE_ARGS+=("${EXTRA_ARGS[@]}")
fi

echo "==> Configuring (Debug + ASan)"
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" "${CMAKE_ARGS[@]}"

PARALLEL=${AUTOGITPULL_BUILD_PARALLEL:-}
if [[ -z "${PARALLEL}" ]]; then
    if command -v sysctl >/dev/null 2>&1; then
        PARALLEL=$(sysctl -n hw.logicalcpu 2>/dev/null || echo 4)
    elif command -v nproc >/dev/null 2>&1; then
        PARALLEL=$(nproc)
    else
        PARALLEL=4
    fi
fi

echo "==> Building (${PARALLEL} jobs)"
cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}" --parallel "${PARALLEL}"

BIN_SUBDIR="${BUILD_DIR}"
if [[ -d "${BUILD_DIR}/${BUILD_TYPE}" ]]; then
    BIN_SUBDIR="${BUILD_DIR}/${BUILD_TYPE}"
fi

EXEC_NAME="autogitpull"
case "${OSTYPE:-}" in
    msys*|cygwin*|win32*) EXEC_NAME="autogitpull.exe" ;;
    *) EXEC_NAME="autogitpull" ;;
esac

if [[ -f "${BIN_SUBDIR}/${EXEC_NAME}" ]]; then
    cmake -E make_directory "${ROOT_DIR}/dist"
    cmake -E copy "${BIN_SUBDIR}/${EXEC_NAME}" "${ROOT_DIR}/dist/${EXEC_NAME}_debug"
    echo "Debug binary copied to dist/${EXEC_NAME}_debug"
else
    echo "Build completed; debug binary located under ${BIN_SUBDIR}" >&2
fi
