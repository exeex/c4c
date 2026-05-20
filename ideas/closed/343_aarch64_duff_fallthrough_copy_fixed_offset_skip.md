# AArch64 Duff Fallthrough Copy Fixed-Offset Skip

Status: Closed
Created: 2026-05-20
Split From: ideas/closed/342_aarch64_duff_do_while_latch_condition_emission.md
Closed: 2026-05-20

## Goal

Repair the AArch64 generated-code residual where Duff fallthrough copy blocks
skip every other fixed short-copy offset after the first fallthrough case, even
though prepared BIR carries consecutive source and destination offsets.

## Why This Exists

Idea 342 repaired the Duff do-while latch condition emission: the latch now
branches on the single post-decrement counter value instead of subtracting
again. The representative `00143` still fails `[RUNTIME_NONZERO]`.

The new first bad fact is in generated fallthrough copy emission. Prepared BIR
shows consecutive copy records such as `+2 -> +2`, `+4 -> +4`, and onward, but
generated AArch64 diverges after the initial fallthrough copy. After copying
`[sp] -> [sp,#78]`, `.LBB1_8` copies `[sp,#4] -> [sp,#82]` where prepared BIR
expects the `+2 -> +2` short copy. Later fallthrough blocks continue skipping
every other short offset.

## In Scope

- Localize where consecutive prepared Duff copy offsets become generated
  AArch64 offsets that advance by four bytes per fallthrough case.
- Repair the semantic lowering, selected memory operand construction,
  fallthrough-case scheduling, or machine-printer input path so short copies
  emit consecutive two-byte fixed offsets.
- Preserve the already repaired Duff latch-condition behavior from idea 342.
- Add or update focused backend coverage for generated AArch64 fixed-offset
  fallthrough copy emission when the prepared copy chain uses consecutive
  short offsets.
- Re-run the representative
  `c_testsuite_aarch64_backend_src_00143_c`.

## Out Of Scope

- Duff do-while latch condition decrement/compare emission repaired by idea
  342.
- Scalar-cast register-source publication repaired by idea 340.
- Expectation, unsupported, runner, timeout, proof-log-policy, or CTest
  registration changes.
- Fixing only `00143`, one `.LBB` label, one block number, one stack offset,
  one source line, or one emitted instruction spelling.
- Broad frame-layout, aggregate, variadic, parser, semantic HIR, or frontend
  rewrites unless fresh evidence proves they own this fixed-offset skip.

## Acceptance Criteria

- The first bad boundary is localized to a concrete prepared-BIR-to-machine or
  generated-AArch64 emission owner for fallthrough copy offsets.
- Focused backend coverage proves a Duff-style fallthrough copy chain emits
  consecutive fixed offsets for short copies when prepared BIR records
  consecutive two-byte offsets.
- Generated AArch64 for the `00143` fallthrough copy chain no longer skips the
  `+2`, `+6`, `+10`, or equivalent every-other-copy offsets.
- `c_testsuite_aarch64_backend_src_00143_c` either passes or is reclassified by
  a new first bad fact after the fixed-offset skip is repaired.
- No expectation, runner, timeout, unsupported classification,
  CTest-registration, or proof-log-policy change is used to claim progress.

## Completion Note

Closed on 2026-05-20 after the fixed-offset skip was localized and repaired.
The repair landed in `src/backend/prealloc/stack_layout/coordinator.cpp`, with
focused backend coverage proving that fallthrough copy chains whose prepared
BIR offsets advance by two bytes no longer skip every other generated AArch64
short-copy offset.

Step 3 re-ran the representative classification. Prepared addressing still
maps consecutive Duff fallthrough slots, and generated AArch64 now emits the
consecutive short-copy offsets `[sp]`, `[sp,#2]`, `[sp,#4]`, and onward. The
Duff latch repair from idea 342 also remains intact: the latch branches on the
single post-decrement counter value.

`c_testsuite_aarch64_backend_src_00143_c` still fails `[RUNTIME_NONZERO]`, but
the remaining first bad fact is outside this idea. Semantic BIR freezes
`*from++` and `*to++` dereferences to direct `%lv.a.0` and `%lv.b.0` base
slots across the loopback instead of consuming the loop-carried `%lv.from` and
`%lv.to` pointer state. That residual was split to
`ideas/open/344_semantic_bir_loop_carried_pointer_deref_provenance.md`.

Close-time regression guard used matched representative logs for
`c_testsuite_aarch64_backend_src_00143_c`. The strict pass-increase check is
not applicable because the representative is intentionally split as a still
failing residual, so the non-decreasing guard was used: before and after both
reported 0 passed and 1 failed out of 1, with no new failing tests.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00143`, `.LBB1_8`, Duff-device labels, one block number, one
  stack offset, one source line, or one exact `ldrh`/`strh` spelling instead of
  repairing fixed-offset copy emission generally;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, or CTest registration to reduce the failure count;
- claims progress only through helper renames, diagnostic rewrites,
  classification notes, or c-testsuite expectation changes;
- reopens the idea-342 latch-condition path without fresh evidence that the
  duplicate latch decrement returned;
- broadens into unrelated frame-layout, aggregate, variadic, parser, semantic
  HIR, or frontend rewrites before proving the fixed-offset skip boundary;
- leaves generated AArch64 still skipping every other prepared short-copy
  offset behind a renamed abstraction.
