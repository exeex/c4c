Status: Active
Source Idea Path: ideas/open/230_aarch64_c_testsuite_backend_full_scan.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Repair Only Easy Scan-Path Failures

# Current Packet

## Just Finished

Completed Step 4, Repair Only Easy Scan-Path Failures.

Build-system repair:

- Added a root CMake guard in `tests/CMakeLists.txt`:
  `ENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN` now requires
  `ENABLE_C4C_BACKEND=ON`.
- Reconfigured `build-aarch64-scan` with `ENABLE_C4C_BACKEND=ON` and rebuilt
  `c4cll`, so the scan no longer fails with `backend support is disabled in
  this build`.
- Updated the root-owned AArch64 runner asm check to accept ordinary assembler
  directives with leading horizontal whitespace, such as `    .text`, while
  still requiring `.text` to be the first non-whitespace token on a line.

Backend-enabled scan counts:

- Registered cases: 220
- Compiled to AArch64 assembly files: 192
- Assembled/linked: 149
- Run-and-matched: 0
- Failed: 220
- Runtime-unavailable: 149

Failure inventory:

- Runtime unavailable bucket: 149 cases.
  - CTest-visible tag: `[RUNTIME_UNAVAILABLE]`.
  - Representative cases:
    `c_testsuite_aarch64_backend_src_00001_c`,
    `c_testsuite_aarch64_backend_src_00002_c`,
    `c_testsuite_aarch64_backend_src_00003_c`,
    `c_testsuite_aarch64_backend_src_00123_c`.
  - These cases produced `.s` and `.bin` files, then stopped honestly because
    no `C_TESTSUITE_AARCH64_BACKEND_RUNNER` was configured for this host.
- Backend assembly/link toolchain bucket: 43 cases.
  - CTest-visible tag: `[BACKEND_FAIL]`.
  - Representative undefined-label cases:
    `src/00005.c`, `src/00006.c`, `src/00007.c`, `src/00156.c`,
    `src/00200.c` report undefined temporary `.LBB*` symbols from clang.
  - Representative invalid-assembly cases:
    `src/00012.c` has invalid operand `sub w0, x19, #8`;
    `src/00018.c` and `src/00021.c` report invalid instruction operands;
    `src/00124.c` reports an expected compatible register/symbol/integer
    range diagnostic.
- AArch64 lowering / machine-printer bucket: 28 cases.
  - CTest-visible tag: `[FRONTEND_FAIL]`.
  - Representative machine-printer cases:
    `src/00024.c` cannot print scalar `sub` with immediate lhs/register rhs;
    `src/00027.c` cannot print scalar `or`;
    `src/00028.c` cannot print scalar `and`;
    `src/00104.c` cannot print scalar `xor`;
    `src/00213.c` cannot print scalar `add` immediate outside `0..4095`.
  - Representative semantic-prepared-BIR cases:
    `src/00140.c`, `src/00143.c`, `src/00176.c`, `src/00185.c`,
    `src/00216.c` fail semantic `lir_to_bir` outside admitted capability
    buckets.

Empty buckets in this scan:

- Frontend parse/sema C-language diagnostics: 0 visible.
- Fallback/non-AArch64 output: 0.
- Runtime mismatch: 0.

## Suggested Next

Choose the next packet from either the backend assembly/link surface
(`.LBB*` labels and invalid AArch64 operands) or the 28 semantic
lowering/printer failures. Runtime validation needs a configured
`C_TESTSUITE_AARCH64_BACKEND_RUNNER` or native AArch64 execution environment.

## Watchouts

- The old `[BACKEND_FALLBACK_IR]` scan-path artifact is gone after relaxing the
  root-owned runner check for indented `.text`.
- The 149 `[RUNTIME_UNAVAILABLE]` cases are not backend successes; they
  reached assembly/link and stopped before runtime comparison.
- The 28 `[FRONTEND_FAIL]` cases expose genuine AArch64 lowering/printer
  capability gaps, but this packet intentionally made no semantic backend
  repairs.
- Do not weaken c-testsuite expectations or add testcase-shaped shortcuts.

## Proof

Proof log: `test_after.log`.

Commands run:

- `cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON`
- `cmake --build build-aarch64-scan --target c4cll -j2`
- `ctest --test-dir build-aarch64-scan --output-on-failure -R '^c_testsuite_aarch64_backend_'`

Results:

- Final ctest exit code: 8.
- `0% tests passed, 220 tests failed out of 220`.
- Visible failure tags: 149 `[RUNTIME_UNAVAILABLE]`, 43 `[BACKEND_FAIL]`,
  28 `[FRONTEND_FAIL]`.
- `find build-aarch64-scan/c_testsuite_aarch64_backend -name '*.s'` found 192
  assembly output files; all 192 contain `.text`.
- `find build-aarch64-scan/c_testsuite_aarch64_backend -name '*.bin'` found
  149 linked binaries.
