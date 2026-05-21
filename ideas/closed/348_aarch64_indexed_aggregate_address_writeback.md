# AArch64 Indexed Aggregate Address And Writeback

Status: Closed
Created: 2026-05-20
Split From: ideas/open/295_backend_regex_failure_family_inventory.md

## Goal

Repair AArch64 generated-code handling for dynamic indexed aggregate and array
addressing when stores, loads, swaps, or member accesses must compute the
element address and write back to the selected aggregate slot rather than to a
stale base, byte offset, or temporary value.

## Why This Exists

The post-347 backend-regex umbrella inventory classified 20 residual external
AArch64 c-testsuite failures. The strongest coherent first-bad-fact bucket is
indexed aggregate addressing/writeback, not a variadic, byval, scalar-cast, or
frame-layout owner.

Representative evidence from the generated AArch64 artifacts:

- `00130.c` stores `arr[1][3]` at `[sp,#2]`, while the later check reads the
  expected element at `[sp,#7]`.
- `00176.c` prints the input array mostly unchanged after quicksort, consistent
  with indexed global array swaps not writing back through the selected
  element addresses.
- `00182.c` loses most LED digits after buffer writes, matching incorrect
  dynamic buffer element placement.
- `00187.c` prints `hch...` because the local buffer terminator lands at the
  wrong byte.
- `00195.c` walks many `point_array+N` addresses but stores `d9` to each and
  then prints `0.000000, 0.000000` instead of the indexed struct fields.
- `00181.c` segfaults in recursive Tower of Hanoi code whose generated shape
  mutates global tower arrays through pointer parameters using fixed global
  snapshots and address-selected stores.

These failures share the same semantic risk: an indexed aggregate operation
has a selected element or member address in the source program, but generated
AArch64 either targets the wrong byte offset, fails to preserve the computed
element address, writes a stale temporary, or collapses a dynamic element
operation into fixed base/global storage.

## In Scope

- Localize the AArch64 path from BIR/prepared aggregate element selection to
  selected address computation, emitted load/store, and writeback.
- Repair dynamic indexed loads, stores, swaps, and member accesses for local
  arrays, global arrays, buffer-like locals, and arrays of structs when the
  element address depends on an index or pointer parameter.
- Preserve the selected element address across helper temporaries, calls, and
  recursive control-flow when that address is the destination or source of the
  aggregate operation.
- Add focused backend coverage for representative indexed aggregate shapes,
  including byte arrays, multidimensional locals, global array swaps, and
  struct-member array elements.
- Use the classified c-testsuite representatives only as external proof after
  focused backend coverage identifies and guards the repaired owner.

## Out Of Scope

- Variadic aggregate `va_arg`, byval aggregate argument lane publication,
  stdarg cursor/format progression, HFA/floating aggregate publication, or
  fixed-formal entry publication unless fresh generated-code evidence reaches
  those exact boundaries.
- Scalar local storage/writeback sizing for non-address-exposed scalar locals,
  scalar cast register-source publication, return-result publication,
  direct-call argument/formal publication, or local conversion store/load
  publication.
- Static aggregate initializer and relocation materialization for `00205.c` or
  `00216.c` unless the first bad fact is proven to be the same dynamic indexed
  address/writeback path.
- Boolean, comparison, floating-point expression, recursive scalar state,
  semantic `lir_to_bir` admission, timeout, runner, expectation,
  unsupported-classification, CTest-registration, or proof-log behavior.
- Fixing only one c-testsuite filename, one array dimension, one byte offset,
  one global symbol, one struct field, one temporary register, or one emitted
  instruction sequence.

## Acceptance Criteria

- The first indexed aggregate bad fact is localized to a concrete prepared
  address, selected address, emitted load/store, or writeback handoff boundary.
- Focused backend coverage fails without the repair and passes with it for
  dynamic indexed aggregate addressing/writeback behavior.
- At least two differently shaped representatives from the classified bucket
  advance past their current first bad fact or pass, unless localization proves
  one representative belongs to a separate owner and records that handoff.
- Existing focused guardrails for variadic/byval aggregates, scalar casts,
  scalar local writeback, return publication, direct-call publication, and
  local conversion publication remain stable in the chosen proof subset.
- Any remaining failures in `00130`, `00176`, `00181`, `00182`, `00187`, or
  `00195` are reclassified by their new first bad fact before this idea is
  closed.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00130`, `00176`, `00181`, `00182`, `00187`, `00195`, one
  array name, one global symbol, one struct type, one source index, one byte
  offset, one temporary slot, one register, or one emitted load/store sequence
  instead of repairing general indexed aggregate address/writeback handling;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, CTest registration, or external test contracts to
  improve counts;
- claims capability progress only through helper renames, diagnostic rewrites,
  classification notes, or c-testsuite expectation changes;
- proves only one named representative while nearby dynamic indexed aggregate
  shapes from the classified bucket remain unexamined;
- folds boolean/comparison materialization, FP expression lowering,
  semantic-admission repairs, static aggregate initializer/relocation work, or
  variadic/byval aggregate call-boundary work into this route without fresh
  generated-code evidence that the first bad fact moved there;
- leaves the old failure mode in place behind a new abstraction, such that
  generated AArch64 still stores through a stale base, wrong byte offset,
  stale temporary, or fixed global snapshot when the source program selected a
  dynamic aggregate element.

## Lifecycle Handoff

2026-05-20: Step 3 repairs made the selected-address/writeback representatives
`00130`, `00187`, and `00195` pass in the delegated subset. The remaining red
representatives from the 348 proof subset were reclassified by new first bad
facts and should not be continued as routine indexed aggregate address
writeback work:

- `00176` still has indexed global swap/writeback symptoms, but current
  generated `partition`/`quicksort` evidence also reuses caller-clobbered
  `w0`/`w1` after recursive `bl` calls. That first bad fact is split to
  `ideas/open/349_aarch64_recursive_call_argument_preservation.md`.
- `00181` now localizes to the same recursive call-boundary preservation
  shape in generated `Hanoi`, not selected aggregate address publication.
  It is split to
  `ideas/open/349_aarch64_recursive_call_argument_preservation.md`.
- `00182` reaches selected global stores for the digit array, but the stored
  scalar value comes from unsigned `urem`/`trunc` and the loop update consumes
  an unsupported `udiv` producer. That first bad fact is split to
  `ideas/open/350_aarch64_unsigned_div_rem_producer_publication.md`.

The active 348 runbook is deactivated rather than closed because this
delegation forbids touching `ideas/closed/` and no close-time regression guard
was requested. Resume 348 only if fresh generated-code evidence again shows an
in-scope dynamic indexed aggregate selected-address/writeback failure after
the residual call-preservation and unsigned div/rem publication owners are
handled.

2026-05-21: Reactivated after
`ideas/open/353_aarch64_local_formal_frame_slot_publication.md` repaired the
formal-to-local publication failure in `00176`. The current `00176` failure is
now a runtime output mismatch, not timeout/stack exhaustion. Fresh generated
evidence points back to in-scope global indexed array snapshot/writeback in
`swap`: high stack snapshot slots such as `[sp, #264]` and `[sp, #268]` are
read back uninitialized and used to corrupt the final array. Treat this as the
next selected-address/writeback localization route, while preserving the prior
348 repairs for `00130`, `00187`, and `00195`.

2026-05-21: Closed after the reactivated selected-snapshot/writeback route
repaired `00176`. The latest focused before/after regression guard improved
from 6/8 to 7/8 by resolving `c_testsuite_aarch64_backend_src_00176_c` with no
new failures; `00130` and `00195` stayed passing, and the supervisor broader
backend guard passed 141/141. The remaining
`c_testsuite_aarch64_backend_src_00187_c` `RUNTIME_NONZERO` segmentation fault
is not an indexed aggregate selected-address/writeback failure in the current
classification. It is split to
`ideas/open/354_aarch64_external_call_symbol_home_publication.md` as external
call argument/symbol home publication work.
