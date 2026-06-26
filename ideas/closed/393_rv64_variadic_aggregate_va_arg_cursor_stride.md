# RV64 Variadic Aggregate Va Arg Cursor Stride

Status: Closed
Type: Target lowering follow-up
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
Split From: `ideas/closed/387_rv64_object_route_same_module_sret_calls.md`
Closed By: RV64 aggregate `va_arg` cursor stride repair plus Step 5 routing evidence

## Goal

Fix the RV64 callee-side variadic aggregate `va_arg` boundary where a
register-passed aggregate payload is consumed from the variadic GPR save area
but advances the cursor by the aggregate object size instead of the ABI
variadic GPR slot stride.

## Why This Exists

Idea 387 fixed the same-module sret call object-emission boundary for
`tests/c/external/gcc_torture/src/920908-1.c`. The representative now compiles,
links, enters the callee, and receives the caller-published values:

- `a2=10`
- `a3=20`

The remaining abort is in callee `f`. c4c saves those values into 8-byte
variadic GPR save-area slots. The first aggregate `va_arg` reads the 32-bit
payload at the start of the first slot and gets `10`. The cursor then advances
by 4 bytes, so the second aggregate `va_arg` reads the high half of the first
8-byte slot and gets `0` instead of advancing to the next 8-byte slot and
reading `20`.

This is not same-module sret call lowering, caller publication, or ordinary
scalar argument lowering. It is callee-side RV64 variadic aggregate
consumption/layout.

## In Scope

- Capture prepared/BIR/object evidence for aggregate `va_arg` over
  register-passed RV64 variadic payloads.
- Identify the authoritative stride/layout rule for advancing across RV64
  variadic GPR save-area slots for aggregate payloads.
- Add focused backend coverage for the accepted cursor advance and adjacent
  fail-closed or unsupported shapes.
- Implement the narrow aggregate `va_arg` cursor/layout repair.
- Rerun `920908-1.c` and route any later boundary separately.

## Out of Scope

- Reopening same-module sret calls from idea 387.
- Reopening caller-side argument publication when qemu shows `a2=10` and
  `a3=20` at callee entry.
- Reopening RV64 variadic prologue save-area publication from idea 391 unless
  fresh evidence shows the register save area is no longer populated.
- Reopening `va_list` expression call-argument publication from idea 392.
- Broad variadic, aggregate, or call-ABI rewrites beyond the aggregate
  `va_arg` cursor stride/layout route.
- Hard-coding `920908-1.c`, callee `f`, literal stack offsets, registers, or
  the abort branch.

## Acceptance Criteria

- The exact prepared/BIR/object fact gap for RV64 aggregate `va_arg` cursor
  stride over variadic GPR save-area slots is identified.
- Focused backend coverage proves that supported aggregate `va_arg` shapes
  advance across ABI variadic GPR slots and read the next payload correctly.
- Unsupported or ambiguous aggregate `va_arg` shapes fail closed with a clear
  owner instead of silently reading the wrong slot.
- `920908-1.c` advances past the current aggregate `va_arg` abort, or any
  remaining later boundary is recorded with a clear owner and split instead of
  expanding this idea.

## Closure Notes

The aggregate `va_arg` cursor-stride acceptance is satisfied. Step 5 evidence
in `build/agent_state/393_step5_analysis.log` shows both aggregate helpers now
carry `payload_size=4`, `copy_size=4`, `source_slot=8`,
`progression_stride=8`, and `overflow_stride=8`.

The representative advances past the owned aggregate boundary:

- the first aggregate read observes `10`
- the second aggregate read observes `20`
- both prior abort branches are skipped

The supervisor full backend proof for the implementation was accepted before
closure: backend CTest remained 326/326 with no new failures. The plan-owner
close gate was rerun as lifecycle-only validation with the accepted current
backend baseline on both sides because `test_after.log` currently contains
representative evidence, not a CTest after-log:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed`

Result: PASS, 326/326 before and 326/326 after, no new failures.

The remaining representative failure is a later same-module sret/callee
`%ret.sret` home-publication boundary. `main` passes the sret address in `a0`,
but callee `f` later loads stack-homed `%ret.sret` from `0(sp)` without a prior
callee-side publication/save of incoming `a0` into the `%ret.sret` home slot.
That boundary is split to
`ideas/open/394_rv64_same_module_sret_callee_home_publication.md`.

## Reviewer Reject Signals

- Reject named-case handling for `920908-1.c`, callee `f`, concrete registers,
  literal stack offsets, or the current abort branch.
- Reject fixes that only change the cursor by aggregate object size without
  proving the ABI variadic GPR slot stride for register-saved aggregate
  payloads.
- Reject caller-publication rewrites when evidence still shows `a2=10` and
  `a3=20` at callee entry.
- Reject expectation rewrites, unsupported downgrades, allowlist filtering, or
  abort suppression claimed as progress.
- Reject broad variadic or aggregate ABI rewrites that do not include focused
  proof for aggregate `va_arg` cursor stride/layout.
