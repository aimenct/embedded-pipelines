#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
CHECK_BUILD_DIR="${EPF_CHECK_BUILD_DIR:-${SCRIPT_DIR}/build/mr-check}"
BUILD_JOBS="${EPF_BUILD_JOBS:-2}"
TIDY_JOBS="${EPF_TIDY_JOBS:-4}"
CHECK_NAME="${1:-all}"

cd "${SCRIPT_DIR}"

require_commands()
{
  for command_name in "$@"; do
    if ! command -v "${command_name}" >/dev/null 2>&1; then
      echo "error: required command not found: ${command_name}" >&2
      exit 1
    fi
  done
}

configure()
{
  require_commands cmake
  echo "==> Configuring ${CHECK_BUILD_DIR}"
  cmake -S . -B "${CHECK_BUILD_DIR}" \
    -DBUILD_TESTING=ON \
    -DENABLE_CAMERA=ON \
    -DENABLE_OPCUA=ON \
    -DENABLE_DOCS=OFF
}

check_format()
{
  require_commands clang-format git
  echo "==> Checking clang-format"
  mapfile -d "" format_files < <(
    git ls-files -z -- \
      "*.c" "*.cc" "*.cpp" "*.cxx" \
      "*.h" "*.hh" "*.hpp" "*.hxx"
  )
  if (("${#format_files[@]}" > 0)); then
    clang-format --dry-run --Werror "${format_files[@]}"
  fi
}

build()
{
  configure
  echo "==> Building library and tests with ${BUILD_JOBS} workers"
  cmake --build "${CHECK_BUILD_DIR}" --target epf tests --parallel "${BUILD_JOBS}"
}

check_tidy()
{
  require_commands run-clang-tidy
  configure
  echo "==> Checking clang-tidy with ${TIDY_JOBS} workers"
  run-clang-tidy -p "${CHECK_BUILD_DIR}" -j "${TIDY_JOBS}" \
    -warnings-as-errors="readability-identifier-naming"
}

run_tests()
{
  build
  echo "==> Running non-display tests"
  ctest \
    --test-dir "${CHECK_BUILD_DIR}" \
    --output-on-failure \
    --no-tests=error \
    -LE display
}

case "${CHECK_NAME}" in
  format)
    check_format
    ;;
  build)
    build
    ;;
  tidy)
    check_tidy
    ;;
  test)
    run_tests
    ;;
  all)
    check_format
    build
    check_tidy
    run_tests
    ;;
  *)
    echo "usage: $0 {format|build|tidy|test|all}" >&2
    exit 2
    ;;
esac

echo "==> ${CHECK_NAME} checks passed"
