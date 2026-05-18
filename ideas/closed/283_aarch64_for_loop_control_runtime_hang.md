# AArch64 For-Loop Control Runtime Hang

Status: Closed
Created: 2026-05-18
Discovered From: ideas/open/276_aarch64_c_testsuite_backend_runtime_execution.md
Closed: 2026-05-18

## Intent

Repair the next AArch64 backend loop/control lowering failure exposed by the
broader c-testsuite backend route: `tests/c/external/c-testsuite/src/00007.c`
reaches backend assembly generation and clang assembler/link, then hangs at
runtime because generated control flow contains unconditional self-loops for a
`for`/loop-control shape.

## Why This Exists

Idea 276 is the AArch64 backend c-testsuite runtime-route and inventory plan.
After the focused backend/codegen repair chain, `00001.c` through `00006.c`
pass through the real backend `.s -> clang -x assembler -> executable ->
runtime -> expected-output` route.

The broader route inventory then selected 220 AArch64 backend tests and
blocked at `00007.c` with `[RUNTIME_HANG]`. Generated `.s` and `.bin` artifacts
exist, so the route reached backend assembly and clang assembler/link. The
remaining blocker is a backend loop/control lowering owner layer, not route
infrastructure.

## In Scope

- Trace the HIR/BIR/MIR or prepared control-flow facts for `00007.c`.
- Identify why the AArch64 backend emits unconditional self-loops for the
  `for`/loop-control shape.
- Repair the semantic branch/loop-control lowering rule for the supported form
  proven by the trace.
- Preserve the already-green AArch64 backend runtime route for `00001.c`
  through `00006.c`.
- Use timeout-bounded runtime proof or assembly-only localization while the
  hang remains active.

## Out of Scope

- Changing c-testsuite `.expected` sidecars, allowlists, unsupported
  classifications, CTest contracts, runner files, or timeout policy to claim
  progress.
- Adding filename-specific handling for `00007.c`.
- Routing proof through LLVM IR fallback while claiming AArch64 backend
  progress.
- Reopening completed scalar local, address-exposed pointer storage, or
  `00005.c`/`00006.c` fused-compare branch repairs unless their exact old
  owner-layer failures return.
- Broad parser, frontend, ABI, register-allocation, or route-runner rewrites
  unless trace evidence proves this loop-control owner requires them.

## Completion Criteria

- `00007.c` no longer hangs due to unconditional self-loops in generated
  AArch64 output.
- Generated control flow contains truthful loop-exit paths for the supported
  `for`/loop-control shape.
- Focused backend proof covers the repaired control-flow rule outside the exact
  c-testsuite filename where practical.
- `00001.c` through `00006.c` remain green through the AArch64 backend runtime
  route.
- Any later runtime or backend failure is classified truthfully and split
  rather than hidden behind timeout, expectation, runner, or allowlist changes.

## Closure Notes

Closed after Step 2 repair and close-gate review. Commits `dd5dfbe80` and
`e3feea5e2` repaired the supported `for`/loop-control owner and fixed back the
prepared branch-record boundary regression. `00007.c` no longer hangs, and
generated AArch64 control flow has truthful conditional loop-exit paths.

Focused backend proof covers divergent prepared/BIR label IDs outside the exact
c-testsuite filename, including prepared branch records and branch-control
lowering. The AArch64 backend runtime route for `00001.c` through `00007.c`
is green with timeout-bounded proof, and `00001.c` through `00006.c` remain
green.

Close-time regression guard passed with regenerated matching-scope logs:
`test_before.log` and `test_after.log` both cover the eight focused AArch64
backend tests plus `c_testsuite_aarch64_backend_src_(00001|00002|00003|00004|00005|00006|00007)_c`,
with 7 passed, 0 failed, and no new failures in the parsed final CTest
summary.

## Reviewer Reject Signals

Reject the route or slice if it:

- matches `00007.c`, a c-testsuite path, or one exact label spelling instead of
  repairing semantic loop/control lowering;
- changes `.expected` files, allowlists, unsupported classifications, CTest
  contracts, runner files, or timeout policy to make the hang disappear;
- routes proof through LLVM IR fallback while claiming AArch64 backend progress;
- fixes only the visible self-loop while leaving equivalent `for` or loop
  control forms unexamined;
- regresses the already-green `00001.c` through `00006.c` backend runtime
  route;
- folds broad route-inventory or unrelated backend/ABI/frontend work into this
  focused loop-control owner.
