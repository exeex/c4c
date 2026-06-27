# RV64 Object Route RHS Bitfield Boolean Select Abort

Status: Open
Type: Runtime follow-up repair idea
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
Split From: `ideas/closed/402_rv64_gcc_torture_runtime_abort_and_segfault_mismatches.md`
Related Prior Work: `ideas/closed/381_rv64_object_route_short_circuit_select_join_materialization.md`

## Goal

Repair the RV64 object-route runtime abort in `20000113-1.c` caused by stale
or mis-scheduled RHS bitfield/boolean value materialization around the
short-circuit select/join path.

## Why This Exists

Idea 402 repaired the distinct `20070212-2.c` frame-slot local-address
segfault family. The remaining runtime representative `20000113-1.c` still
compiles, links, and reaches qemu, but diverges from clang:

```text
clang_qemu_status=0
c4c_qemu_status=134
c4c_qemu_strace_status=134
```

The strace ends in `SIGIOT` after reaching `abort()`. Prepared evidence around
`foobar` includes RHS bitfield/boolean materialization values
`%t43/%t44/%t48` and a select/join fact:

```text
join_transfer logic.end.40 result=%t48 kind=phi_edge
carrier=select_materialization ownership=authoritative_branch_pair
```

Object evidence indicates the abort path is reached after stale or
mis-scheduled value handling around the RHS bitfield/boolean materialization
and select/join path. This is not the repaired 402 frame-slot local-address
family and should not be folded back into generic runtime triage.

## In Scope

- Classify the `20000113-1.c` abort around `foobar`, `%t43/%t44/%t48`, and the
  `logic.end.40` select/join materialization path.
- Inspect prepared select/join facts, value homes, object scheduling, and
  emitted branch/value materialization around the RHS bitfield/boolean path.
- Repair the first supportable RV64 object value scheduling/control-flow
  family that causes the abort.
- Preserve observable qemu behavior against clang for the representative.
- Route any producer-fact defect or distinct runtime residual to a separate
  owner instead of masking it in RV64 object emission.

## Out Of Scope

- Reopening the 402 frame-slot local-address repair for `20070212-2.c`.
- Reopening closed idea 381 unless fresh evidence proves the exact prior
  short-circuit select/join materialization rule regressed.
- Broad runtime mismatch triage unrelated to the `20000113-1.c` RHS
  bitfield/boolean select path.
- Compile-only proof for a case that reaches qemu.
- Expectation rewrites, qemu comparison weakening, allowlist filtering, or
  testcase-specific shortcuts.

## Acceptance

- `src/20000113-1.c` no longer reaches the same runtime abort from stale or
  mis-scheduled RHS bitfield/boolean select/join materialization.
- Focused backend/object-route coverage proves the repaired value
  scheduling/control-flow family and preserves adjacent behavior.
- The representative is rerun through clang-vs-c4c qemu comparison; success or
  any later residual is documented with precise ownership.
- Existing backend and prior select/join/local-address coverage remains green.

## Reviewer Reject Signals

- Reject filename-specific handling for `20000113-1.c`, function-specific
  handling for `foobar`, exact value ids such as `%t43/%t44/%t48`, or exact
  block labels such as `logic.end.40`.
- Reject fixes that only avoid `abort()` without preserving clang-equivalent
  observable behavior.
- Reject compile-only proof for this runtime owner.
- Reject masking missing or incorrect prepared select/join facts in RV64
  object emission if the proof shows the producer is wrong.
- Reject expectation downgrades, qemu weakening, allowlist filtering, or
  diagnostic-only churn claimed as runtime progress.
