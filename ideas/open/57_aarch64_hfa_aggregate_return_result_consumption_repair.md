# AArch64 HFA Aggregate Return Result Consumption Repair

## Goal

Repair AArch64 HFA aggregate call-result publication and caller-side
consumption so each returned FP lane is consumed from the ABI result registers
published by the callee and the prepared `AfterCall` boundary, not from stale
long-lived FPR homes.

## Why This Exists

Reactivating idea 56 showed that the focused eight-test subset now passes seven
tests and fails only `c_testsuite_aarch64_backend_src_00204_c`. The first
remaining divergence is not in the historical variadic edge/terminator path.
In straight-line `ret()`, `fr_hfa12()` returns and prints `2.0 12.1` instead
of `12.1 12.2`.

The generated `fr_hfa12` callee publishes both HFA lanes to ABI result
registers `s0` and `s1`, but the caller side after `bl fr_hfa12` stores and
converts stale `s9`/`s13`. Existing prepared tests already expect HFA
call-result lanes to publish distinct `AfterCall` ABI bindings for `s0` and
`s1`; this idea owns the gap between those prepared facts and the caller
lowering that stores or otherwise consumes the aggregate result.

## In Scope

- AArch64 HFA aggregate call-result lanes returned in FP/SIMD ABI result
  registers.
- Prepared `AfterCall` move bundles and ABI bindings that identify `s0`, `s1`,
  and higher HFA result lanes as call-result sources.
- AArch64 caller-side consumers that store, copy, convert, or materialize an
  HFA aggregate result immediately after a call.
- Minimal prepared lookup or module-record plumbing only when existing
  `PreparedMoveBundle`, `AbiBindingRecord`, `CallResultRecord`, or value-home
  facts cannot be consumed directly.

Likely owned implementation surfaces:

- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/mir/aarch64/module/module.cpp`
- `src/backend/mir/aarch64/module/module.hpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/cast_ops.cpp`

Likely proof surfaces:

- `tests/backend/bir/backend_prepare_liveness_test.cpp`
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- `tests/backend/mir/backend_aarch64_target_instruction_records_test.cpp`
- `tests/backend/mir/backend_aarch64_machine_printer_test.cpp`
- `c_testsuite_aarch64_backend_src_00204_c`

## Out Of Scope

- Do not repair idea 56 edge/terminator consumer preservation here.
- Do not change AAPCS64 argument staging, variadic register-save area layout,
  va_list cursor writeback, or by-value argument publication under this idea.
- Do not change callee-side `BeforeReturn` HFA lane publication unless tracing
  proves the callee failed to publish the ABI result lanes.
- Do not add named-case logic for `00204`, `ret`, `fr_hfa12`, `s9`, or `s13`.
- Do not weaken expectations, mark supported probes unsupported, or claim
  classification-only notes as capability progress.

## Acceptance Criteria

- The caller-side first bad fact is traced to a concrete consumer that ignores
  prepared HFA call-result lane publication after `bl fr_hfa12`.
- HFA aggregate result consumers use the prepared ABI result lane source
  registers, including `s0`/`s1` for the observed two-lane case, instead of
  stale allocated FPR homes.
- Existing prepared HFA call-result lane tests remain active and either pass as
  written or are strengthened without expectation downgrades.
- `c_testsuite_aarch64_backend_src_00204_c` no longer prints the observed
  `fr_hfa12()` mismatch for semantic reasons, not by testcase-shaped filtering.
- After this route is repaired, any remaining `00204` failure is classified to
  the next precise owner before implementation expands into a different
  subsystem.

## Reviewer Reject Signals

- Reject named-case fixes for `00204`, `ret`, `fr_hfa12`, `s9`, `s13`, or the
  exact two-lane fixture instead of a general HFA call-result consumption rule.
- Reject changes that only make the callee publish `s0`/`s1` again while the
  caller still consumes stale post-call FPR homes.
- Reject raw assembly text suppression, BIR name matching, or same-block scans
  used as the durable source of call-result lane authority when prepared
  `AfterCall` moves, ABI bindings, call-result records, or value homes exist.
- Reject changes that fold edge/terminator preservation, variadic va_list
  layout, or by-value argument publication work into this route without a
  separate lifecycle decision.
- Reject expectation downgrades, unsupported-test rewrites, helper renames, or
  classification-only changes claimed as capability progress.
- Reject broad AArch64 call or memory rewrites that do not prove HFA aggregate
  result consumers now read the ABI result lanes published at the call
  boundary.
