Status: Active
Source Idea Path: ideas/open/230_aarch64_c_testsuite_backend_full_scan.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Run The First Full Scan

# Current Packet

## Just Finished

Completed Step 3, Run The First Full Scan.

Ran the explicit AArch64 c-testsuite backend scan without semantic repairs:

- Registered cases: 220
- Compiled to AArch64 assembly: 0
- Assembled/linked: 0
- Run-and-matched: 0
- Failed: 220
- Runtime-unavailable: 0

Failure inventory:

- Toolchain / build-configuration blocker: 220 cases.
  - CTest-visible tag: `[FRONTEND_FAIL]`.
  - Representative cases:
    `c_testsuite_aarch64_backend_src_00001_c`,
    `c_testsuite_aarch64_backend_src_00002_c`,
    `c_testsuite_aarch64_backend_src_00003_c`,
    `c_testsuite_aarch64_backend_src_00204_c`.
  - Representative setup probe:
    `./build-aarch64-scan/c4cll --codegen asm --target aarch64-unknown-linux-gnu tests/c/external/c-testsuite/src/00001.c -o /tmp/c4c_aarch64_probe_00001.s`
    exited 2 with `backend support is disabled in this build`.

Empty buckets in this scan:

- Frontend parse/sema semantic diagnostics: 0 visible beyond the runner's
  `[FRONTEND_FAIL]` wrapper.
- BIR/prepared-BIR: 0.
- AArch64 lowering: 0.
- Fallback/non-AArch64 output: 0.
- Backend assembly/link toolchain: 0.
- Runtime mismatch: 0.
- Runtime-unavailable: 0.

The scan did not reach source-level backend capability failures because this
configured `c4cll` binary rejects `--codegen asm` before writing any output.

## Suggested Next

Enable or configure the backend-native asm route in the AArch64 scan build, then
rerun the same explicit scan to produce a semantic backend failure inventory.

## Watchouts

- `build-aarch64-scan/c4cll` was missing before the first ctest attempt; the
  `c4cll` target was built successfully, then the final scan was rerun into
  `test_after.log`.
- The final scan is a valid failure inventory for the current build
  configuration, but it is not yet an inventory of AArch64 lowering failures.
- Do not weaken c-testsuite expectations to make this scan green; the current
  blocker is that backend-native asm codegen is disabled in the scan build.

## Proof

Proof log: `test_after.log`.

Commands run:

- `ctest --test-dir build-aarch64-scan --output-on-failure -R '^c_testsuite_aarch64_backend_' > test_after.log 2>&1`
- Setup check after the first attempt found the test compiler missing:
  `cmake --build build-aarch64-scan --target c4cll -j2`
- Final proof rerun:
  `ctest --test-dir build-aarch64-scan --output-on-failure -R '^c_testsuite_aarch64_backend_' > test_after.log 2>&1`
- Representative classifier probe recorded in `test_after.log`:
  `./build-aarch64-scan/c4cll --codegen asm --target aarch64-unknown-linux-gnu tests/c/external/c-testsuite/src/00001.c -o /tmp/c4c_aarch64_probe_00001.s`

Results:

- Final ctest exit code: 8.
- `0% tests passed, 220 tests failed out of 220`.
- All 220 failures have CTest-visible `[FRONTEND_FAIL]` from
  `c-testsuite-aarch64-backend-runner.cmake`.
- No output files were created under `build-aarch64-scan/c_testsuite_aarch64_backend`.
- The classifier probe shows the underlying first-stage diagnostic:
  `backend support is disabled in this build`.
