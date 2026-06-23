# RV64 I16 Local Array Select Store Publication

## Goal

Repair RV64 prepared lowering for halfword local-array select/store publication
when an i16 local-array body follows an already-emitted stack-homed fused
compare loop branch.

## Why This Exists

Idea 319 Step 5 reclassified the remaining `src/00143.c` failure as outside
the stack-homed fused compare control-flow boundary. Fresh evidence shows the
first loop condition emits its stack-homed fused compare branch and false
successor jump, then output truncates later inside `.Lmain_block_1` before the
false successor label is defined. Prepared body evidence points to the
i32-to-i64 index cast followed by i16 local-array select/store publication,
including the `%t9.store0` family.

This is distinct from idea 320, which tracks the `src/00144.c` nested
select-chain/store-source publication residual. This idea is specifically for
halfword local-array select/store lowering exposed by `src/00143.c` after
control-flow emission has already progressed past the loop condition.

## In Scope

- RV64 prepared lowering for i16 local-array select/store publication.
- Halfword local-array stores reached after a stack-homed fused compare loop
  branch has already emitted.
- Destination/source publication facts for the `%t9.store0`-style local-array
  select/store family exposed by `src/00143.c`.
- Focused backend coverage that separates halfword local-array select/store
  body emission from stack-homed fused compare control flow.

## Out Of Scope

- Stack-homed fused compare branch or loop-condition repair from idea 319.
- Nested select-chain/store-source publication for `src/00144.c`, tracked by
  idea 320.
- Pointer-to-pointer local frame-address materialization from idea 316.
- Aggregate/byval repair, function-pointer repair, external-call policy, or
  broad switch/control-flow work.

## Acceptance Criteria

- Focused tests cover RV64 i16 local-array select/store publication without
  depending on `src/00143.c` filenames, block labels, SSA names, or observed
  stack offsets.
- `src/00143.c` either emits, links, and runs under qemu, or any remaining
  failure is reclassified with concrete emitted-code evidence as a different
  mechanism after the i16 local-array select/store body advances.
- Generated assembly no longer truncates inside the first loop body before
  reaching required successor labels solely because the halfword local-array
  select/store publication is missing.
- Repairs consume or improve prepared local-array/select/store publication
  facts rather than weakening runtime contracts or marking the candidate
  unsupported.

## Completion Note

Closed on 2026-06-23 after Step 3/4 i16 halfword repair satisfied the source
idea criteria. Focused i16 local-array select/store dump/codegen/runtime
coverage is positive and preserves halfword semantics with `lh`/`sh`.

`src/00143.c` was reprobed under
`build/rv64_c_testsuite_probe_latest/triage_321_step3/`. BIR dump,
prepared-BIR dump, RV64 emit, and clang link all returned 0. The emitted RV64
now contains semantic halfword select/store bodies for the former i16
local-array publication boundary. qemu exits 132 because `.Lmain_block_2` is an
empty loop-exit successor at the end of emitted text and falls into the next
section. That is a separate control-flow/empty-exit residual, now tracked by
`ideas/open/322_rv64_empty_loop_exit_successor_emission.md`.

Close gate: backend regression guard passed with
`test_before.log` versus `test_after.log` using
`--allow-non-decreasing-passed`: 278 passed, 1 failed before and after, with
the existing `backend_riscv_prepared_edge_publication` failure unchanged.

## Reviewer Reject Signals

- The implementation special-cases `src/00143.c`, `.Lmain_block_1`,
  `.Lmain_block_2`, `%t9.store0`, or fixed stack offsets instead of repairing
  i16 local-array select/store publication.
- Progress is claimed by changing the focused fixture from i16/halfword storage
  to a wider integer type without a separate reclassification note.
- The route claims stack-homed fused compare control-flow progress while the
  remaining failure is still in the later local-array select/store body.
- The patch overlaps idea 320 by repairing only `src/00144.c` nested ternary
  store-source facts while `src/00143.c` halfword local-array publication
  remains unchanged.
- Runtime or link proof is weakened, skipped, or replaced with unsupported
  expectations for a candidate that should remain on the supported path.
