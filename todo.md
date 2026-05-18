Status: Active
Source Idea Path: ideas/open/276_aarch64_c_testsuite_backend_runtime_execution.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Current Runner And Registration

# Current Packet

## Just Finished

Completed Step 1: audited the current c-testsuite frontend/backend runner and
registration route for AArch64 backend runtime execution.

Relevant files inspected:
- `tests/CMakeLists.txt`
- `tests/c/external/CMakeLists.txt`
- `tests/c/external/c-testsuite/RunCase.cmake`
- `tests/c/external/c-testsuite-aarch64-backend-runner.cmake`
- `tests/c/external/c-testsuite/allowlist.txt`

Audit result:
- The frontend c-testsuite route still uses `c4cll SRC | clang -x ir -` and
  runs the host binary against the `.expected` sidecar.
- The shared backend branch in `RunCase.cmake` already invokes
  `c4cll --codegen asm --target ${TARGET_TRIPLE}`, writes `.s`, checks for
  assembler-looking output, assembles/links with clang, and runs the binary.
- The AArch64-specific runner already requires
  `CODEGEN_MODE=backend-aarch64` and
  `TARGET_TRIPLE=aarch64-unknown-linux-gnu`, checks backend `.s` output,
  assembles with `clang --target=aarch64-unknown-linux-gnu -x assembler`, runs
  natively on AArch64 hosts, or parses `C_TESTSUITE_AARCH64_BACKEND_RUNNER` as
  a command plus arguments and appends the linked binary.
- `tests/c/external/CMakeLists.txt` already registers explicit
  `c_testsuite_aarch64_backend_*` tests with label `aarch64_backend` when
  `ENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON`, independent of host backend
  auto-selection.
- The first missing implementation work is not basic registration or runner
  command parsing. It is focused proof materialization: make the representative
  AArch64 backend runtime subset for `src/00001.c`, `src/00002.c`, and
  `src/00003.c` explicit enough to run and review as the route's first proof.

## Suggested Next

First implementation packet: execute Step 3 by editing
`tests/c/external/CMakeLists.txt` to add a focused AArch64 backend runtime
proof surface for the existing registered cases
`c_testsuite_aarch64_backend_src_00001_c`,
`c_testsuite_aarch64_backend_src_00002_c`, and
`c_testsuite_aarch64_backend_src_00003_c`.

Recommended shape: keep the existing full `aarch64_backend` scan intact, but
add a narrow label such as `aarch64_backend_smoke` or an equivalent focused
CTest grouping only for those three cases. Do not edit the allowlist,
`.expected` files, or runner semantics in this packet unless the focused proof
cannot be expressed without a small CMake registration helper.

## Watchouts

- A passing test must use AArch64 backend `.s`, not LLVM IR fallback.
- Runtime-unavailable is not a pass.
- Do not weaken expected outputs or shrink allowlists to get green.
- Do not add named-case AArch64 lowering shortcuts while working on the runner.
- Preserve the existing full-scan registration under label `aarch64_backend`;
  the focused proof should be additive and narrow.
- The recommended proof requires an AArch64 runtime path on non-AArch64 hosts;
  configure `C_TESTSUITE_AARCH64_BACKEND_RUNNER` to a real command such as a
  qemu-user invocation with any required sysroot arguments.
- If the focused cases fail after route registration is explicit, classify the
  first failure stage rather than changing expectations: `[FRONTEND_FAIL]`,
  `[BACKEND_OUTPUT_MISSING]`, `[BACKEND_FALLBACK_IR]`, `[BACKEND_FAIL]`,
  `[RUNTIME_UNAVAILABLE]`, `[RUNTIME_NONZERO]`, or `[RUNTIME_MISMATCH]`.

## Proof

Audit-only packet. No build/test proof was required, no tests were run, and no
root-level proof log was created.

Recommended proof command for the next implementation packet:

```sh
cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -L aarch64_backend_smoke
```
