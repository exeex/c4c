# RV64 Loop-Carried Pointer Post-Increment Publication

## Goal

Repair RV64 prepared lowering/publication for loop-carried pointer
post-increment values across Duff's-device-style fallthrough blocks so repeated
iterations advance from current `from`/`to` pointer values instead of
rematerializing fixed array-base offsets.

## Why This Exists

Idea 322 Step 4 reprobed `src/00143.c` after empty loop-exit successor
emission was repaired. The old fallthrough/SIGILL is gone: `.Lmain_block_2`,
the Duff's-device switch/fallthrough body blocks, the verification loop, and
return blocks are emitted, and qemu exits normally with status 1.

The first remaining wrong-result evidence is loop-carried pointer
post-increment publication/lowering. The Duff's-device copy blocks repeatedly
store fixed stack addresses back into `%lv.from` and `%lv.to` homes, such as
`addi t1, sp, 2; sd t1, 160(sp)` and `addi t1, sp, 80; sd t1, 168(sp)` in
`.Lmain_block_6`, with similar fixed offsets in following fallthrough blocks.
Prepared-BIR shows the same shape, for example
`%t29 = bir.add ptr %lv.a.0, 2; bir.store_local %lv.from, ptr %t29`. Repeated
iterations therefore restart from array-base-derived addresses instead of
advancing from the current loop-carried `from`/`to` values.

## In Scope

- RV64 prepared lowering/publication for loop-carried pointer post-increment
  values.
- Pointer local updates across Duff's-device-style fallthrough blocks and
  repeated loop iterations.
- Focused backend coverage that proves pointer post-increment locals advance
  from current values rather than fixed array-base offsets.
- Candidate reprobe for `src/00143.c` after empty-exit successor emission and
  i16 local-array select/store publication have already advanced.

## Out Of Scope

- Empty loop-exit successor emission from idea 322.
- I16 local-array select/store publication from idea 321.
- Stack-homed fused compare control flow from idea 319.
- Nested select-chain/store-source publication for `src/00144.c` from idea
  320.
- Aggregate/byval repair, function-pointer repair, external-call policy, or
  broad switch/control-flow rewrites beyond loop-carried pointer
  post-increment publication.

## Acceptance Criteria

- Focused tests cover loop-carried pointer post-increment locals across
  fallthrough/repeated loop iterations without depending on `src/00143.c`,
  Duff's-device spelling, block names, stack offsets, or SSA names.
- `src/00143.c` either emits, links, and runs under qemu, or any remaining
  failure is reclassified with concrete emitted-code evidence as a different
  mechanism after loop-carried pointer updates advance correctly.
- Generated assembly updates pointer locals from current loop-carried values,
  not fixed array-base rematerializations, in focused proof cases.
- Repairs consume or improve prepared pointer/local publication facts rather
  than target-local source scans or testcase-shaped fallback matching.

## Completion Note

Closed on 2026-06-23 after Step 5 reprobe showed the focused loop-carried
pointer post-increment residual was repaired. The focused
`backend_(dump|codegen_route|rv64_runtime)_riscv64_loop_carried_pointer_postincrement`
coverage is positive. In `src/00143.c`, Duff `block_6` now loads current
`%lv.from`/`%lv.to`, computes next pointer values with `addi ... 2`, stores
the updated pointer locals, and copies the halfword through the old pointer
value.

`src/00143.c` still emits and links but qemu exits 139. The remaining failure
is classified as `separate-duff-fallthrough-pointer-update-publication-residual`.
The first bad block is `block_9`: BIR stores `%t39` and `%t42` into
`%lv.from`/`%lv.to`, but there are no `bir.add ptr` producer instructions for
those next-pointer values. Prepared-BIR gives `%t39` and `%t42` stack homes
with `source_producer=unknown`, and emitted RV64 loads uninitialized stack
homes before publishing them as pointer locals. That residual is outside idea
323's repaired current-pointer consumption path and is tracked by
`ideas/open/324_rv64_duff_fallthrough_pointer_update_producers.md`.

Close gate: existing accepted backend guard state passed with
`test_before.log` compared to itself using `--allow-non-decreasing-passed`:
284 passed, 1 failed before and after, with the existing
`backend_riscv_prepared_edge_publication` failure unchanged.

## Reviewer Reject Signals

- The implementation special-cases `src/00143.c`, Duff's-device block layout,
  `%lv.from`, `%lv.to`, `%t29`, fixed stack offsets, or fixed array sizes
  instead of repairing loop-carried pointer post-increment publication.
- Progress is claimed by changing the candidate/focused fixture to avoid
  repeated fallthrough loop iterations rather than preserving current pointer
  values.
- The route reopens empty-exit successor, i16 halfword local-array, or
  stack-homed fused compare repairs even though current evidence shows those
  boundaries already advanced.
- Runtime proof is weakened, skipped, or replaced with unsupported
  expectations for a candidate that remains on the supported path.
- Broad control-flow rewrites are mixed in without focused proof that pointer
  locals advance from current loop-carried values.
