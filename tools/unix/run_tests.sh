#!/usr/bin/env bash

set -euo pipefail

readonly SCRIPT_NAME=$(basename "$0")
readonly LOG=$(mktemp "/tmp/${SCRIPT_NAME}.XXXXXX")
readonly SMOKE_SUITE=(  \
  base_tests            \
  coding_tests          \
  generator_tests       \
  indexer_tests         \
  map_tests             \
  mwm_tests             \
  platform_tests        \
  routing_tests         \
  search_tests          \
)
readonly OMIM_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
readonly DATA_PATH="${OMIM_ROOT}/data"
readonly TEST_SERVER_DIR="${OMIM_ROOT}/tools/python/test_server"
BUILD_DIR=.
SUITE=full
EXCLUDE=
FILTER=
TEST_SERVER_STARTED=0

export TZ=UTC
export LC_ALL=C
export LANG=C

log() {
  echo "$@" 2>&1 | tee -a "$LOG"
}

die() {
  log "$@"
  echo "Terminated. Log is written to $LOG"
  exit 1
}

start_test_server() {
  if [ "$TEST_SERVER_STARTED" -eq 0 ]; then
    (cd "$TEST_SERVER_DIR" && python3 start_server.py)
    TEST_SERVER_STARTED=1
    sleep 1
  fi
}

stop_test_server() {
  if [ "$TEST_SERVER_STARTED" -eq 1 ]; then
    (cd "$TEST_SERVER_DIR" && python3 stop_server.py) || true
    TEST_SERVER_STARTED=0
  fi
}

trap stop_test_server EXIT

run_test_binary() {
  local testBin=$1
  if [ -n "$FILTER" ]; then
    ./"$testBin" --data_path="$DATA_PATH" --user_resource_path="$DATA_PATH" --filter="$FILTER"
  else
    ./"$testBin" --data_path="$DATA_PATH" --user_resource_path="$DATA_PATH"
  fi
}

usage() {
  log "Usage: $0 [options]"
  log "Options:"
  log "  -b    path to build directory, default: ."
  log "  -s    test suite, smoke or full, default: full"
  log "  -f    regular expression which is applied to all tests, default: .*"
  log "  -e    regular expression which is applied to test binaries, default: none"
  log "  -h    prints this help message"
  log ""
  log "Smoke test suite consists of:"
  for testName in "${SMOKE_SUITE[@]}"
  do
      log "  " "$testName"
  done
  exit 1
}

while [ $# -ne 0 ]
do
  case "$1" in
    -b) BUILD_DIR=${2?"Build directory is not set"}
        shift
        ;;
    -s) SUITE=${2?"Suite name is not set"}
        shift
        ;;
    -f) FILTER=${2?"Test filter regex is not set"}
        shift
        ;;
    -e) EXCLUDE=${2?"Exclude filter regex is not set"}
        shift
        ;;
    -h) usage
        ;;
  esac
  shift
done

if [ ! -d "$BUILD_DIR" ]
then
  die "Build directory $BUILD_DIR does not exists"
fi

cd "$BUILD_DIR"

case "$SUITE" in
  smoke) TESTS=("${SMOKE_SUITE[@]}")
         ;;
   full) TESTS=($(find . -maxdepth 1 -name '*_tests'))
         ;;
      *) die "Unknown test suite: $SUITE"
         ;;
esac

PASSED_TESTS=0
TOTAL_TESTS=0
for testBin in "${TESTS[@]}"
do
  if [ "$EXCLUDE" ] && [[ "$testBin" =~ $EXCLUDE ]]
  then
    continue
  fi

  if [ ! -x "$testBin" ]
  then
    die "Can't find test $testBin"
  fi

  TOTAL_TESTS=$((TOTAL_TESTS + 1))

  log "Running $testBin..."
  if [ "$testBin" = "platform_tests" ]; then
    start_test_server
  fi
  (run_test_binary "$testBin" 2>&1 | tee -a "$LOG") && ((PASSED_TESTS++)) || true
done

log "$PASSED_TESTS / $TOTAL_TESTS passed."
echo "Log is written to: $LOG"
if [ "$TOTAL_TESTS" -ne "$PASSED_TESTS" ]; then
  exit 1
fi
