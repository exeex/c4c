Status: Active
Source Idea Path: ideas/open/353_c_testsuite_rv64_asm_objdump_roundtrip_scan_target.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Prove And Document The Opt-In Scan

# Current Packet

## Just Finished

Step 5 documented and reproved the opt-in RV64 c-testsuite asm/objdump
roundtrip scan. `tests/backend/rv64/rv64_c_testsuite_asm_roundtrip_scan.md`
now gives the manual `cmake --build build --target
rv64_c_testsuite_asm_roundtrip_scan` command, states that the scan is a custom
target rather than default CTest membership, and points maintainers at the
allowlist and unsupported-family note before expanding coverage.

## Suggested Next

The runbook appears complete. Suggested next packet: supervisor should review
the completed Step 1 through Step 5 slice, inspect the final diff for route
drift, and decide whether to commit or ask the plan owner to close/deactivate
the active plan state.

## Watchouts

- The scan target remains opt-in and should stay out of default full CTest.
- The CTest membership check intentionally writes to
  `/tmp/rv64_c_testsuite_scan_ctest_membership.txt`; an empty file means no
  registered scan test was found.
- The initial allowlist remains intentionally tiny: `src/00001.c` and
  `src/00002.c`.

## Proof

Delegated Step 5 proof passed and wrote `test_after.log`:

```sh
cmake --build build --target rv64_c_testsuite_asm_roundtrip_scan >test_after.log 2>&1 && ctest --test-dir build -N | rg 'rv64_c_testsuite_asm_roundtrip_scan|c_testsuite_asm_roundtrip_scan' > /tmp/rv64_c_testsuite_scan_ctest_membership.txt || true
```

`test_after.log` shows the opt-in target passing both allowlisted cases and
finishing with `[PASS][rv64-c-testsuite-roundtrip-scan] cases=2`.
`/tmp/rv64_c_testsuite_scan_ctest_membership.txt` is empty, confirming that
`ctest -N` did not list a registered scan test.
