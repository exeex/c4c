Status: Active
Source Idea Path: ideas/open/230_aarch64_c_testsuite_backend_full_scan.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Existing Registration

# Current Packet

## Just Finished

Completed Step 1 inspection for c-testsuite registration and backend runner
wiring.

Current registration flow:

- `tests/CMakeLists.txt` declares `C_TESTSUITE_ROOT` and
  `ENABLE_C_TESTSUITE_BACKEND_TESTS` (default `OFF`), auto-detects
  `tests/c/external/c-testsuite`, and unconditionally includes
  `tests/c/external`.
- `tests/c/external/CMakeLists.txt` reads
  `tests/c/external/c-testsuite/allowlist.txt`; each non-comment entry
  registers a frontend test named `c_testsuite_${test_id}` with label
  `c_testsuite`.
- Backend tests are registered only when
  `ENABLE_C_TESTSUITE_BACKEND_TESTS`, `C_TESTSUITE_BACKEND_MODE`,
  `C_TESTSUITE_BACKEND_LABEL`, and `C_TESTSUITE_BACKEND_TRIPLE` are all set.
  They are named `c_testsuite_${C_TESTSUITE_BACKEND_ID}_backend_${test_id}`
  and labeled `c_testsuite;${C_TESTSUITE_BACKEND_LABEL}`.

Current backend target/host behavior:

- `tests/c/external/CMakeLists.txt` derives the backend route from
  `CMAKE_HOST_SYSTEM_PROCESSOR`.
- On `x86_64`/`amd64`, backend registration is
  `backend-x86_64`, triple `x86_64-unknown-linux-gnu`, id `x86`, label
  `x86_backend`, output dir `c_testsuite_x86_backend`.
- On `aarch64`/`arm64`, backend registration is
  `backend-aarch64`, triple `aarch64-unknown-linux-gnu`, id `aarch64`,
  label `aarch64_backend`, output dir `c_testsuite_aarch64_backend`.
- Other hosts leave the backend variables empty, so no backend tests register.
- In the current `build/CMakeCache.txt`,
  `ENABLE_C_TESTSUITE_BACKEND_TESTS:BOOL=OFF`; `ctest --test-dir build -N -R
  c_testsuite` lists 220 frontend c-testsuite tests, while
  `ctest --test-dir build -N -R 'c_testsuite_.*backend'` lists 0 backend tests.

Current runner behavior:

- `tests/c/external/c-testsuite/RunCase.cmake` defaults missing
  `CODEGEN_MODE` to `frontend`.
- Frontend mode pipes `c4cll SRC` into `clang -x ir - ...` and runs the
  produced host binary directly.
- Backend mode accepts any `CODEGEN_MODE` matching `^backend-`, requires
  `OUT_LL` and `TARGET_TRIPLE`, runs
  `c4cll --codegen asm --target TARGET_TRIPLE SRC -o OUT_LL`, rejects missing
  output or output whose first 256 bytes lack a `.text` line, assembles/links
  with `clang --target=TARGET_TRIPLE -x assembler OUT_LL -o OUT_BIN -lm`, then
  runs `OUT_BIN` directly and compares stdout/stderr against `SRC.expected`.
- `BACKEND_ASM_SOURCE` is passed by CMake but currently unused by
  `RunCase.cmake`.

Likely first registration edits for an explicit AArch64 backend scan:

- In `tests/CMakeLists.txt`, add cache-level AArch64 scan controls rather than
  reusing the host-only backend switch, for example an explicit enable option
  and optional runtime runner/toolchain knobs.
- In `tests/c/external/CMakeLists.txt`, add a separate AArch64 backend
  registration route that is not derived from `CMAKE_HOST_SYSTEM_PROCESSOR`:
  mode `backend-aarch64`, triple `aarch64-unknown-linux-gnu`, id/label
  `aarch64_backend`, output dir `c_testsuite_aarch64_backend`, and test names
  that remain distinguishable from host backend tests.
- In `tests/c/external/c-testsuite/RunCase.cmake`, split backend compile,
  assemble/link, and runtime execution enough that AArch64 cross output can be
  inventoried honestly. At minimum, add explicit handling for an AArch64
  assembler/linker command and avoid treating direct execution failure on a
  non-AArch64 host as a backend pass.
- Keep existing host-backend registration behavior intact so enabling the
  explicit AArch64 scan does not change current `x86_backend` or native
  `aarch64_backend` behavior by accident.

## Suggested Next

Implement the first narrow registration slice: add explicit AArch64 c-testsuite
backend scan registration controls and thread them through
`tests/c/external/CMakeLists.txt` to produce `aarch64_backend` ctest entries
without changing the existing host-derived backend route.

## Watchouts

- Current backend target selection is host-coupled; enabling
  `ENABLE_C_TESTSUITE_BACKEND_TESTS` on an x86_64 host would register only
  `x86_backend`, not `aarch64_backend`.
- `RunCase.cmake` currently runs every produced binary directly. For an
  explicit AArch64 cross scan, direct runtime must be gated behind a native
  AArch64 host or configured runner/emulator; otherwise compile/assemble/link
  inventory will be conflated with runtime availability.
- `clang --target=aarch64-unknown-linux-gnu` may need explicit sysroot/linker
  handling before link results are meaningful on the current host.
- The `.text` fallback check is useful but weak; it proves "some assembly-like
  path" more than "AArch64 assembly". The first slice can keep it, but the scan
  should eventually record stronger target-output evidence.
- Do not weaken c-testsuite expectations or classify runtime-unavailable cases
  as backend passes.

## Proof

Inspection-only packet; no build/test proof required and no `test_after.log`
was produced.

Non-mutating inspection commands run:

- `ctest --test-dir build -N -R c_testsuite`
- `ctest --test-dir build -N -R 'c_testsuite_.*backend'`
- focused `rg`/`sed`/`nl` inspection of `tests/CMakeLists.txt`,
  `tests/c/external/CMakeLists.txt`, and
  `tests/c/external/c-testsuite/RunCase.cmake`

Recommended narrow proof for the first registration change:

- Configure a fresh or existing build with the new explicit AArch64 scan option
  enabled, then run `ctest --test-dir build -N -R
  '^c_testsuite_aarch64_backend_'` and verify listed commands include
  `-DCODEGEN_MODE=backend-aarch64`,
  `-DTARGET_TRIPLE=aarch64-unknown-linux-gnu`, and outputs under
  `c_testsuite_aarch64_backend/`.
