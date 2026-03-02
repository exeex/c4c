#!/bin/bash
# Quick progress check: run allowlist only, print summary
cd "$(dirname "$0")/.."
WORKDIR=/tmp/c4agent_check

python3.14 tests/run_c_testsuite.py \
    --compiler src/frontend/c2ll.py \
    --clang /usr/bin/clang \
    --testsuite-root tests/c-testsuite \
    --workdir "$WORKDIR" \
    --allowlist tests/c_testsuite_allowlist.txt 2>&1 | grep -E "SUMMARY|FAIL|PASS"

# Save last result
python3.14 tests/run_c_testsuite.py \
    --compiler src/frontend/c2ll.py \
    --clang /usr/bin/clang \
    --testsuite-root tests/c-testsuite \
    --workdir "$WORKDIR" \
    --allowlist tests/c_testsuite_allowlist.txt 2>&1 | grep SUMMARY > build/agent_state/last_result.txt

echo "[check_progress] Done. See build/agent_state/last_result.txt"
