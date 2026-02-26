#!/bin/bash
# Full scan: run ALL 220 tests, find newly passing ones
cd "$(dirname "$0")/.."
WORKDIR=/tmp/c4agent_fullscan

echo "[full_scan] Running all 220 tests... (takes ~60s)"

python3.14 tests/run_c_testsuite.py \
    --compiler src/c2ll.py \
    --clang /usr/bin/clang \
    --testsuite-root tests/c-testsuite \
    --workdir "$WORKDIR" 2>&1 | tee /tmp/full_scan_raw.txt | grep SUMMARY

# Extract passing tests
grep "^\[PASS\]" /tmp/full_scan_raw.txt | awk '{print $2}' | sort > /tmp/full_scan_passing.txt

# Find newly passing (not yet in allowlist)
comm -23 /tmp/full_scan_passing.txt <(sort tests/c_testsuite_allowlist.txt) \
    > build/agent_state/newly_passing.txt

NEWLY=$(wc -l < build/agent_state/newly_passing.txt | tr -d ' ')
echo "[full_scan] Newly passing tests not in allowlist: $NEWLY"
if [ "$NEWLY" -gt 0 ]; then
    echo "[full_scan] See build/agent_state/newly_passing.txt"
    cat build/agent_state/newly_passing.txt
fi
