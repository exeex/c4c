Status: Active
Source Idea Path: ideas/open/353_c_testsuite_rv64_asm_objdump_roundtrip_scan_target.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Existing Test Surfaces

# Current Packet

## Just Finished

Step 1 inspected the existing c-testsuite and backend RV64 roundtrip test
surfaces. The smallest next route is a backend CMake-script runner invoked
directly first, with opt-in CMake target wiring deferred to Step 3.

## Suggested Next

Step 2: Add the per-case roundtrip runner as
`tests/backend/cmake/run_rv64_c_testsuite_asm_roundtrip_scan.cmake`, with a
small explicit initial allowlist such as
`tests/backend/rv64/rv64_c_testsuite_asm_roundtrip_allowlist.txt` if the runner
needs a scan-specific manifest. Owned files for the next executor packet:
`tests/backend/cmake/run_rv64_c_testsuite_asm_roundtrip_scan.cmake`,
optionally `tests/backend/rv64/rv64_c_testsuite_asm_roundtrip_allowlist.txt`,
and `todo.md`.

Narrow proof command for Step 2:

```sh
cmake --build build --target c4cll c4c-as c4c-objdump && cmake -DC4CLL="$PWD/build/c4cll" -DC4C_AS="$PWD/build/c4c-as" -DC4C_OBJDUMP="$PWD/build/c4c-objdump" -DROOT="$PWD" -DC_TESTSUITE_ROOT="$PWD/tests/c/external/c-testsuite" -DALLOWLIST="$PWD/tests/backend/rv64/rv64_c_testsuite_asm_roundtrip_allowlist.txt" -DWORK_DIR="$PWD/build/rv64_c_testsuite_asm_roundtrip_scan" -DCASE_TIMEOUT_SEC=10 -P "$PWD/tests/backend/cmake/run_rv64_c_testsuite_asm_roundtrip_scan.cmake" >test_after.log 2>&1
```

## Watchouts

- Keep the large scan opt-in and out of default full CTest.
- Report unsupported generated assembly explicitly; do not silently skip or
  rewrite unsupported lines.
- The scan must use `c4cll --codegen asm`, `c4c-as`, and `c4c-objdump`; do not
  substitute external assembler or objdump tools as the source of truth.
- Reuse the RV64 contract script's pass naming and comparisons:
  `pass1.s == pass2.s` and `pass2.o == pass3.o`.
- Reuse c-testsuite allowlist parsing conventions: strip blank/comment lines
  and treat entries as paths relative to `tests/c/external/c-testsuite/`.
- Keep Step 2 as direct runner proof only; do not register default CTest
  membership until the explicit opt-in integration step.

## Proof

Read-only inspection plus this `todo.md` update. No build proof was required
for the planning-inspection packet, and no `test_after.log` was produced.
