Status: Active
Source Idea Path: ideas/open/353_c_testsuite_rv64_asm_objdump_roundtrip_scan_target.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add The Per-Case Roundtrip Runner

# Current Packet

## Just Finished

Step 2 tightened the direct per-case RV64 c-testsuite asm/objdump roundtrip
runner diagnostics before acceptance. Text-stability and object-stability
failures now report `case=<path>` and `stage=comparison` while also naming the
specific failed comparison as `comparison=pass1.s == pass2.s` or
`comparison=pass2.o == pass3.o`.

## Suggested Next

Step 3: Wire an explicit opt-in CMake target for the scan without adding it to
default CTest membership. Suggested owned files are `tests/backend/CMakeLists.txt`
and `todo.md`; include the new runner and allowlist as read-only inputs unless
the integration exposes a runner issue.

Suggested proof should build `c4cll`, `c4c-as`, and `c4c-objdump`, then invoke
the new opt-in target and confirm it drives the same runner against
`tests/backend/rv64/rv64_c_testsuite_asm_roundtrip_allowlist.txt`.

## Watchouts

- Keep the large scan opt-in and out of default full CTest.
- The initial scan allowlist is intentionally tiny: `src/00001.c` and
  `src/00002.c`.
- `src/00003.c` currently reaches `c4c-as-pass1` but fails because generated
  RV64 assembly contains unsupported pseudo-instruction `mv a0, s1`; keep this
  as an explicit exclusion until assembler or printer support changes.
- Step 3 should pass tool paths with `$<TARGET_FILE:c4cll>`,
  `$<TARGET_FILE:c4c-as>`, and `$<TARGET_FILE:c4c-objdump>`, but should not add
  the large scan to default full CTest.

## Proof

Delegated proof passed after diagnostic tightening and wrote `test_after.log`:

```sh
cmake --build build --target c4cll c4c-as c4c-objdump && cmake -DC4CLL="$PWD/build/c4cll" -DC4C_AS="$PWD/build/c4c-as" -DC4C_OBJDUMP="$PWD/build/c4c-objdump" -DROOT="$PWD" -DC_TESTSUITE_ROOT="$PWD/tests/c/external/c-testsuite" -DALLOWLIST="$PWD/tests/backend/rv64/rv64_c_testsuite_asm_roundtrip_allowlist.txt" -DWORK_DIR="$PWD/build/rv64_c_testsuite_asm_roundtrip_scan" -DCASE_TIMEOUT_SEC=10 -P "$PWD/tests/backend/cmake/run_rv64_c_testsuite_asm_roundtrip_scan.cmake" >test_after.log 2>&1
```

`test_after.log` reports both allowlisted cases passing all source-idea stages
and finishes with `[PASS][rv64-c-testsuite-roundtrip-scan] cases=2`.
