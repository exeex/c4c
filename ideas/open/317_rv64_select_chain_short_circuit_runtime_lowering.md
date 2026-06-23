# RV64 Select Chain Short Circuit Runtime Lowering

## Goal

Repair RV64 prepared lowering for select-chain and short-circuit control/value
flow when generated code emits and links but computes the wrong runtime result.

## Why This Exists

Idea 314 repaired the aggregate-local subobject mechanism for `src/00046.c`.
Step 5 evidence in
`build/rv64_c_testsuite_probe_latest/triage_314_step5/summary.md` shows the
candidate now emits aggregate stores/reloads at the expected local offsets, but
qemu exits `2`. The remaining first bad mechanism is a separate select-chain /
short-circuit runtime lowering residual, not nested aggregate store emission.

## In Scope

- RV64 prepared select-chain materialization and short-circuit runtime lowering.
- Control/value joins that feed select-chain results in emitted RV64 code.
- Focused backend coverage for select-chain and short-circuit semantics,
  including the shape exposed by `src/00046.c`.
- Emitted-code evidence that distinguishes control/select failure from local
  aggregate storage failure.

## Out Of Scope

- Reopening aggregate-local subobject stores/loads repaired by idea 314.
- Generic local frame-slot address publication from idea 312.
- Broad boolean/control-flow rewrites unrelated to select-chain or
  short-circuit materialization.
- Byval aggregate call ABI, vararg ABI, or floating aggregate lane repair.

## Acceptance Criteria

- Focused tests cover RV64 select-chain and short-circuit runtime semantics.
- `src/00046.c` either emits, links, and exits under qemu with status `0`, or
  any remaining failure is reclassified with concrete emitted-code evidence as
  a different mechanism.
- Repairs consume or improve prepared select-chain/control facts rather than
  matching the c-testsuite filename, fixed aggregate offsets, or expression
  spelling.
- Existing aggregate-local focused tests remain green.

## Reviewer Reject Signals

- The route special-cases `src/00046.c`, fixed offsets, or the exact source
  expression instead of repairing select-chain or short-circuit lowering.
- Progress is claimed by weakening the qemu contract, marking the test
  unsupported, or editing expectations without a runtime semantic repair.
- The patch changes aggregate-local storage again without proving the remaining
  first bad mechanism is aggregate storage rather than select-chain control.
- The implementation adds target-local scans that duplicate prepared
  select-chain facts while leaving the semantic failure unchanged.
- Only one named testcase is proven while nearby select-chain or short-circuit
  shapes remain unexamined.
