# RV64 Stack Homed Fused Compare Control Flow

## Goal

Repair RV64 prepared control-flow emission when branch or loop conditions are
fused compare values whose homes are stack slots rather than immediately
available registers.

## Why This Exists

Idea 316 Step 5 evidence classified `src/00077.c` and `src/00143.c` as a
separate stack-homed fused compare/control-flow residual. In `src/00077.c`,
`foo` emits only through `lw s2, 0(a0)` and then falls through to `main` with
no conditional branch or return; prepared facts show the entry branch condition
`%t5 = ne i32 %t4, 1000` is stack-homed. In `src/00143.c`, emission stops in
the first loop condition after spilling `%t0`, before the large array/select
body is reached; prepared facts show `%t0` and fused compare `%t1` are
stack-homed before the branch.

## In Scope

- RV64 prepared branch and loop-condition emission for stack-homed fused
  compare values.
- Loading or materializing stack-homed compare operands/results for control
  flow without falling through into the next function or truncating assembly.
- Focused backend coverage for stack-homed fused compare control flow,
  including the shapes exposed by `src/00077.c` and `src/00143.c`.
- Emitted-code and qemu proof that distinguishes compare/control-flow repair
  from local-array or pointer publication repair.

## Out Of Scope

- Reopening pointer-to-pointer local frame-address materialization from idea
  316.
- Nested select-chain/store-source publication residuals from `src/00144.c`.
- Broad switch lowering beyond what is necessary for stack-homed fused compare
  branch or loop conditions.
- Aggregate/byval repair, function-pointer repair, or external-call policy.

## Acceptance Criteria

- Focused tests cover RV64 branch/loop emission where a fused compare condition
  is stack-homed.
- `src/00077.c` and `src/00143.c` either emit, link, and run under qemu, or any
  remaining failures are reclassified with concrete emitted-code evidence as
  different mechanisms.
- Repairs use prepared compare/control-flow facts and stack-home information
  rather than filename, stack-offset, or source-shape matching.
- Generated assembly does not fall through into the next function or truncate
  before emitting the required branch/return path for the focused cases.

## Completion Note

Closed on 2026-06-23 after Step 5 reclassification. Focused stack-homed fused
compare dump/codegen/runtime coverage passes. Fresh evidence shows
`src/00077.c` emits, links, and exits qemu with status 0.

`src/00143.c` still fails link with an undefined `.Lmain_block_2`, but the
emitted assembly has already emitted the stack-homed fused compare loop branch
and truncates later inside `.Lmain_block_1`. Prepared evidence identifies the
next body operations as the i32-to-i64 index cast plus i16 local-array
select/store publication (`%t9.store0` family). That residual is outside this
idea's compare/control-flow boundary and is tracked separately in
`ideas/open/321_rv64_i16_local_array_select_store_publication.md`.

Close gate: backend regression guard passed with
`test_before.log` versus `test_after.log` using
`--allow-non-decreasing-passed`: 274 passed, 1 failed before and after, with
the existing `backend_riscv_prepared_edge_publication` failure unchanged.

## Reviewer Reject Signals

- The implementation special-cases `src/00077.c`, `src/00143.c`, `%t5`, `%t1`,
  or observed stack offsets instead of repairing stack-homed fused compare
  control-flow emission.
- Progress is claimed by skipping qemu, weakening runtime contracts, or marking
  the candidates unsupported.
- The route repairs later local-array/select code while the first stack-homed
  compare branch still truncates or falls through.
- The patch duplicates prepared control facts with target-local source scans
  and leaves the stack-homed compare branch behavior unchanged.
- Only one named candidate is proven while nearby stack-homed fused compare
  branch or loop shapes remain unexamined.
