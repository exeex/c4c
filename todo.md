Status: Active
Source Idea Path: ideas/open/230_aarch64_c_testsuite_backend_full_scan.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Honest AArch64 Backend Scan Wiring

# Current Packet

## Just Finished

Completed Step 2, Add Honest AArch64 Backend Scan Wiring.

Added explicit opt-in registration controls:

- `ENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN` defaults `OFF` and registers an
  AArch64 backend scan independently of the existing host-derived
  `ENABLE_C_TESTSUITE_BACKEND_TESTS` route.
- `C_TESTSUITE_AARCH64_BACKEND_RUNNER` is an optional runner/emulator command
  for non-AArch64 hosts.

Updated c-testsuite backend registration:

- Factored backend add-test wiring into a helper so the legacy host-derived
  route keeps the same naming, labels, mode, triple, and output directories.
- The explicit AArch64 scan registers tests named
  `c_testsuite_aarch64_backend_*` with `CODEGEN_MODE=backend-aarch64`,
  `TARGET_TRIPLE=aarch64-unknown-linux-gnu`, label `aarch64_backend`, and
  outputs under `c_testsuite_aarch64_backend/`.
- If native AArch64 host backend tests are already registered through the
  legacy host route, the explicit route does not duplicate the same test names.

Updated explicit AArch64 runtime handling with root-owned wiring:

- Added root-owned
  `tests/c/external/c-testsuite-aarch64-backend-runner.cmake` for the explicit
  AArch64 backend scan.
- Legacy host-derived backend tests still route through the pinned submodule
  runner at `tests/c/external/c-testsuite/RunCase.cmake`.
- The explicit AArch64 runner compiles, checks assembly output, assembles/links,
  then runs directly only on native AArch64 hosts or through
  `C_TESTSUITE_AARCH64_BACKEND_RUNNER`.
- `C_TESTSUITE_AARCH64_BACKEND_RUNNER` is parsed as a command string with
  arguments before appending the produced binary, so runners such as
  `qemu-aarch64 -L /usr/aarch64-linux-gnu` become proper argv lists.
- On non-AArch64 hosts without a runner, the explicit path fails with
  `[RUNTIME_UNAVAILABLE]` instead of silently trying to execute an AArch64
  binary directly.
- Removed the prior dirty submodule runner edit; `tests/c/external/c-testsuite`
  is clean again.

## Suggested Next

Run the explicit AArch64 scan far enough to inventory compile/link failures and
decide the next backend capability packet from real failing cases.

## Watchouts

- The registration proof used `ctest -N -V` only. It proves wiring, not that
  AArch64 compile/link/runtime cases pass.
- On non-AArch64 hosts, running explicit AArch64 backend tests without
  `C_TESTSUITE_AARCH64_BACKEND_RUNNER` is expected to fail at runtime with
  `[RUNTIME_UNAVAILABLE]` after any compile/link success.

## Proof

Proof log: `test_after.log`.

Commands run:

- `cmake -S . -B build-aarch64-scan -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON`
- `ctest --test-dir build-aarch64-scan -N -V -R '^c_testsuite_aarch64_backend_'`
- `cmake -S . -B build-host-backend-check -DENABLE_C_TESTSUITE_BACKEND_TESTS=ON`
- `ctest --test-dir build-host-backend-check -N -V -R '^c_testsuite_aarch64_backend_'`
- `git status --short`

Results:

- Explicit AArch64 backend scan registration listed 220 tests.
- Listed commands include `-DCODEGEN_MODE=backend-aarch64`,
  `-DTARGET_TRIPLE=aarch64-unknown-linux-gnu`, and `OUT_LL`/`OUT_BIN` under
  `build-aarch64-scan/c_testsuite_aarch64_backend/`.
- Listed explicit AArch64 commands use the root-owned
  `tests/c/external/c-testsuite-aarch64-backend-runner.cmake` script.
- The root-owned runner parses `C_TESTSUITE_AARCH64_BACKEND_RUNNER` command
  strings with arguments before runtime execution.
- This workspace reports `CMAKE_HOST_SYSTEM_PROCESSOR=aarch64`; with only the
  legacy host backend option enabled, `^c_testsuite_aarch64_backend_` listed
  220 tests through the original submodule `RunCase.cmake`, confirming the
  host-derived route remains registered.
- `git status --short` does not show lowercase `m tests/c/external/c-testsuite`;
  the nested submodule checkout is clean.
