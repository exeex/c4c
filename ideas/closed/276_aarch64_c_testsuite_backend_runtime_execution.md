# AArch64 C-Testsuite Backend Runtime Execution

Status: Closed
Created: 2026-05-18
Closed: 2026-05-18

## Intent

Make the AArch64 backend run the vendored
`tests/c/external/c-testsuite` cases through the native backend assembly path:

```text
c4cll --codegen asm --target aarch64-unknown-linux-gnu case.c -o case.s
clang --target=aarch64-unknown-linux-gnu -x assembler case.s -o case.bin -lm
<runner or native host> case.bin
compare stdout/stderr with case.c.expected
```

The goal is a real backend execution route for AArch64 `.s` output, analogous
to the existing frontend c-testsuite route that emits LLVM IR and pipes it to
clang.

## Why This Exists

The AArch64 backend now has enough MIR/codegen structure to stop proving only
small hand-picked backend cases. The next useful pressure source is the C
external c-testsuite corpus, but it must exercise the actual AArch64 backend
assembly path rather than the LLVM IR frontend path.

Earlier scan work registered an AArch64 backend c-testsuite route and produced
a failure inventory, but the runtime side was still blocked by
`[RUNTIME_UNAVAILABLE]` on non-AArch64 hosts unless an explicit runner was
configured. This idea turns that route into an intentional execution workflow
with clear compile, assemble/link, run, and expected-output stages.

## Current Starting Point

Existing harness pieces:

- `tests/c/external/c-testsuite/RunCase.cmake` already supports
  `CODEGEN_MODE=backend-*` and can emit backend assembly, assemble/link with
  clang, and run the binary.
- `tests/c/external/c-testsuite-aarch64-backend-runner.cmake` already provides
  an AArch64-specific route that requires
  `CODEGEN_MODE=backend-aarch64`.
- `tests/c/external/CMakeLists.txt` can register explicit AArch64 backend scan
  tests with `ENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON`.
- `C_TESTSUITE_AARCH64_BACKEND_RUNNER` exists as a configurable runtime
  command for non-AArch64 hosts.

Existing gap:

- The route needs to be made acceptance-quality for runtime execution, not just
  scan/classification.
- Runner configuration, diagnostics, proof subsets, and expected pass/fail
  accounting need to be explicit.
- Passing the route must mean the backend produced `.s`, clang produced an
  executable, and runtime output matched the c-testsuite expected sidecar.

## In Scope

- Audit the existing frontend c-testsuite LLVM IR flow and the current AArch64
  backend runner flow.
- Make the AArch64 backend c-testsuite runtime route easy to configure and
  run from CMake/CTest.
- Ensure every registered backend runtime case:
  - invokes `c4cll --codegen asm --target aarch64-unknown-linux-gnu`;
  - writes an assembly file under the build tree;
  - rejects fallback LLVM IR or missing assembly output;
  - invokes clang with `--target=aarch64-unknown-linux-gnu -x assembler`;
  - produces an executable;
  - runs directly on AArch64 hosts or through
    `C_TESTSUITE_AARCH64_BACKEND_RUNNER`;
  - compares output to the existing `.expected` sidecar.
- Preserve separate diagnostics for frontend/BIR failure, backend assembly
  failure, assembler/link failure, runtime unavailable, runtime nonzero, and
  runtime mismatch.
- Start with a small representative allowlist/subset, then make the full
  registered c-testsuite AArch64 backend route runnable.
- Use the route to expose real AArch64 backend capability gaps without adding
  testcase-shaped shortcuts.

## Out of Scope

- Making the full c-testsuite corpus pass in one idea.
- Weakening c-testsuite expected output or allowlist entries to claim progress.
- Counting runtime-unavailable as a pass.
- Hiding missing semantic BIR facts inside AArch64 codegen.
- Adding filename-specific lowering or printer hacks.
- Reworking the x86 backend route.
- Replacing the external clang/as runtime route with an in-process assembler.

## Suggested Execution Order

1. Audit current c-testsuite frontend and backend CMake runner behavior.
2. Normalize/document the AArch64 runtime runner contract, including the exact
   invocation shape for `C_TESTSUITE_AARCH64_BACKEND_RUNNER`.
3. Add a focused AArch64 backend runtime proof target or CTest subset that uses
   representative simple c-testsuite cases such as `src/00001.c`,
   `src/00002.c`, and `src/00003.c`.
4. Prove that a passing case really went through `.s -> clang -> executable ->
   expected-output runtime`, not LLVM IR fallback.
5. Run the broader registered AArch64 backend c-testsuite route and preserve a
   failure summary grouped by stage.
6. If failures require backend/codegen/BIR repairs, split them into focused
   follow-up ideas rather than patching named c-testsuite files.

## Completion Criteria

- There is a clear CMake/CTest route for AArch64 backend c-testsuite runtime
  execution.
- Representative c-testsuite cases compile to AArch64 `.s`, assemble/link with
  clang, run through the configured native/runner path, and match expected
  output.
- The route rejects LLVM IR fallback and missing `.s` output.
- Runtime-unavailable, runtime mismatch, nonzero exit, backend failure, and
  assembler/link failure remain distinct failure classes.
- A broader AArch64 backend c-testsuite scan can be run after the focused proof
  to produce actionable follow-up evidence.

## Closure Notes

Closed after the route-readiness reactivation and Step 3 split review. The
focused AArch64 backend runtime route is green through
`c_testsuite_aarch64_backend_src_(00001|00002|00003|00004|00005|00006)_c`:
the route emits backend AArch64 assembly, assembles/links with clang, runs the
binary, and compares expected output without LLVM IR fallback, expectation
weakening, unsupported reclassification, allowlist changes, or runner edits.

The broader registered AArch64 backend scan selected 220 tests and then
blocked at `00007.c` with `[RUNTIME_HANG]` after passing `00001.c` through
`00006.c`. Generated `.s` and `.bin` artifacts exist for `00007.c`, so the
route reached backend assembly generation and clang assembler/link; the
remaining blocker is a backend loop/control capability gap, not route
infrastructure. That follow-up is tracked by
`ideas/open/283_aarch64_for_loop_control_runtime_hang.md`.

Close-time regression guard passed with matching-scope logs:
`test_before.log` and `test_after.log` both cover
`c_testsuite_aarch64_backend_src_(00001|00002|00003|00004|00005|00006)_c`,
with 6 passed, 0 failed, and no new failures.

## Lifecycle Parking Note

Plan-owner review on 2026-05-18 rejected closure because the current host has
no configured `C_TESTSUITE_AARCH64_BACKEND_RUNNER`, so the focused smoke cases
reach the runtime boundary as `[RUNTIME_UNAVAILABLE]` but do not run and match
expected output. Runtime-unavailable remains a blocker, not a pass, and the
route must not be claimed as backend capability repair.

The active runbook was deactivated after completing route registration and
failure-inventory work. Re-activate this idea when an AArch64 host or runner is
available, then rerun the focused smoke route and a broader AArch64 backend
c-testsuite scan to decide whether the runtime execution criteria are actually
satisfied.

The latest broader inventory selected 220 `aarch64_backend` tests and grouped
the failures as 121 `[RUNTIME_UNAVAILABLE]`, 85 `[FRONTEND_FAIL]`, and 14
`[BACKEND_FAIL]`. Non-runtime capability repairs should be split into focused
follow-up ideas rather than folded into this route-readiness plan.

## Lifecycle Deactivation Note

Plan-owner switch on 2026-05-18 deactivated the runtime-route runbook after
Step 3 proved the route is no longer blocked at runtime availability on this
AArch64 host. `src/00001.c` and `src/00002.c` passed the full backend
`.s -> clang -x assembler -> executable -> runtime -> expected-output` path.
`src/00003.c` reached runtime truthfully and failed as `[RUNTIME_NONZERO]
exit=1`; the generated assembly shape was `sub w19, w0, #4; ret`, returning
without placing the result in `w0`.

That failure is a separate AArch64 backend result-register/codegen issue, not
runtime-route infrastructure. It was split into
`ideas/open/277_aarch64_backend_result_register_runtime_nonzero.md` instead of
expanding this route-readiness plan. Re-activate this idea after the focused
backend-codegen repair if the broader route inventory or closure review for
idea 276 is still needed.

## Reviewer Reject Signals

Reject the route or slice if it:

- exercises the LLVM IR frontend path while claiming AArch64 backend runtime
  coverage;
- counts runtime-unavailable cases as passes;
- weakens `.expected` files, allowlists, or CTest expectations to make the
  route look green;
- adds testcase-shaped AArch64 lowering or printer shortcuts for named
  c-testsuite files;
- collapses compile, assemble/link, runtime, and expected-output failures into
  one vague failure class;
- requires x86 backend changes for the AArch64 route;
- claims corpus progress without preserving representative failing cases and
  their owner layer.
