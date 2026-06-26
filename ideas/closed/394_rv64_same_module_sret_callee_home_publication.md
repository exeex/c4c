# RV64 Same-Module Sret Callee Home Publication

Status: Closed
Type: Target lowering follow-up
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
Split From: `ideas/closed/393_rv64_variadic_aggregate_va_arg_cursor_stride.md`
Closed By: RV64 same-module sret callee home-publication repair plus Step 5 acceptance classification

## Goal

Fix the RV64 same-module callee-side `sret_param` home-publication boundary
where the caller passes the structure-return address in `a0`, but the callee
later reads a stack-homed `%ret.sret` pointer slot that was never initialized
from the incoming ABI register.

## Why This Exists

Idea 393 fixed the aggregate `va_arg` cursor stride boundary in
`tests/c/external/gcc_torture/src/920908-1.c`. Step 5 evidence shows both
aggregate reads now use 8-byte RV64 variadic GPR slots and observe the expected
values:

- first aggregate read: `10`
- second aggregate read: `20`

The representative now fails later with a segmentation fault. The remaining
fault is in callee `f` after both aggregate comparisons pass. `main` passes the
same-module sret address in `a0`, but `f` later emits the return-object store
through stack-homed `%ret.sret`, loading it from `0(sp)` without first saving
or publishing incoming `a0` into that home slot.

This is not aggregate `va_arg` stride/layout, caller-side same-module sret
object emission, or general variadic GPR save-area publication. It is the
callee-side home-publication rule for an incoming same-module RV64 `sret_param`
that is later used as a pointer-value memory base.

## In Scope

- Capture prepared/BIR/object evidence for callee `%ret.sret`, incoming `a0`,
  its stack home slot, and the final pointer-value return store.
- Identify the authoritative rule for publishing an incoming RV64 sret pointer
  into a callee stack-homed `sret_param` object before local-memory use.
- Add focused backend coverage for the supported same-module callee
  `sret_param` home-publication shape.
- Preserve existing same-module caller memory-return object emission from idea
  387.
- Preserve aggregate `va_arg` cursor stride behavior from idea 393.
- Rerun `920908-1.c` and route any later boundary separately.

## Out of Scope

- Reopening same-module caller-side sret object emission from idea 387.
- Reopening RV64 aggregate `va_arg` cursor stride/layout from idea 393.
- Reopening RV64 variadic prologue save-area publication from idea 391.
- Reopening `va_list` expression call-argument publication from idea 392.
- Broad call ABI, prologue, or frame-layout rewrites beyond the callee
  `sret_param` home-publication route.
- Hard-coding `920908-1.c`, callee `f`, concrete registers, literal stack
  offsets, or the final store sequence.

## Acceptance Criteria

- The exact prepared/BIR/object fact gap for callee-side same-module RV64
  `sret_param` home-publication is identified.
- Focused backend coverage proves that supported callee `sret_param` stack
  homes receive the incoming ABI sret pointer before pointer-value local-memory
  use.
- Unsupported or ambiguous sret home-publication shapes fail closed with a
  clear owner instead of silently reading an uninitialized home slot.
- `920908-1.c` advances past the current `%ret.sret` home-publication
  segmentation fault, or any remaining later boundary is recorded with a clear
  owner and split instead of expanding this idea.

## Closure Notes

The same-module RV64 callee `%ret.sret` home-publication acceptance is
satisfied. Step 4 implementation commit `1154aca4` normalized RV64 prepared
pointer `sret_param` homes to exact ABI pointer storage before frame-slot
assignment and made RV64 object emission publish incoming `a0` into supported
sret homes while fail-closing malformed homes.

Focused coverage was added for:

- RV64 narrow and wide returned-aggregate metadata normalizing to 8-byte
  pointer homes
- AArch64 preserving the shared BIR layout
- RV64 object emission and rejection behavior for supported and malformed sret
  homes

Representative evidence confirms the owned boundary is repaired:

- `build/agent_state/394_step4_final_920908-1.prepared.log` still shows BIR
  `ptr sret(size=4, align=4) %ret.sret`, while prepared object/frame-slot facts
  use `source_kind=sret_param type=ptr size=8 align=8`, slot #0 `size=8
  align=8`, and preservation homes with `stack_size=8 stack_align=8`
- `build/agent_state/394_step4_final_920908-1.case.log` reports
  `[PASS][rv64-gcc-torture-backend-obj]` for
  `tests/c/external/gcc_torture/src/920908-1.c`
- Step 5 commit `64b6d62a` classified the post-repair representative as having
  no later observed boundary

Supervisor acceptance ran backend CTest after the implementation and compared
against the accepted baseline with the regression guard. The accepted
`test_after.log` was rolled into `test_before.log` before closure.

The plan-owner close gate was rerun as lifecycle-only validation against the
accepted current backend baseline on both sides:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed`

Result: PASS, 326/326 before and 326/326 after, no new failures.

## Reviewer Reject Signals

- Reject named-case handling for `920908-1.c`, callee `f`, concrete registers,
  literal stack offsets, or the observed return-store instruction sequence.
- Reject caller-only rewrites when evidence shows `main` already passes the
  sret address in `a0` and the fault is a callee home load from an uninitialized
  stack slot.
- Reject fixes that make the return store avoid `%ret.sret` without proving the
  incoming sret pointer is published into the callee home expected by prepared
  facts.
- Reject expectation rewrites, unsupported downgrades, allowlist filtering, or
  abort/segfault suppression claimed as progress.
- Reject broad call ABI or frame rewrites that do not include focused proof for
  same-module callee `sret_param` home-publication.
