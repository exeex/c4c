# AArch64 Va Start Destination Address Materialization

Status: Open
Created: 2026-05-19
Split From: ideas/closed/321_aarch64_aggregate_va_arg_helper_lowering.md

## Goal

Materialize the AArch64 `va_start` destination as a writable local `va_list`
object address before emitted `va_start` field stores use it.

## Why This Exists

Idea 321 lowered aggregate `va_arg` helper records into executable source,
copy, and progression code. The focused `00204.c` representative no longer
contains raw `va.arg.aggregate*` text and now reaches runtime, but still fails
with a segmentation fault.

The current first bad fact is in generated `myprintf`: the `va_start` prologue
stores `__stack`, `__gr_top`, `__vr_top`, `__gr_offs`, and `__vr_offs` through
`x21` before `x21` is materialized as a valid local `va_list` address. The
owning surface is AArch64 `VaStart` destination address publication and
frame-slot materialization, not aggregate `va_arg` source/copy/progression
lowering.

## In Scope

- Localize how `homes.destination_va_list` is represented for AArch64
  `VaStart` records and why the printed base register can be uninitialized.
- Repair `VaStart` lowering so the destination is materialized as a writable
  local `va_list` address before field stores execute.
- Reuse existing AArch64 frame-slot, stack-offset, register, and address
  materialization helpers where possible.
- Add focused backend coverage that proves `va_start` field stores use a
  materialized destination address and do not rely on an uninitialized register.
- Use `c_testsuite_aarch64_backend_src_00204_c` as the representative external
  runtime proof after prior helper-text blockers remain cleared.

## Out Of Scope

- Reopening idea 321's aggregate `va_arg` helper source, copy, or progression
  lowering.
- Reopening idea 320's F128 transport addressability owner.
- Reopening idea 319's HFA, floating, long-double, or aggregate argument
  classification and call-lane lowering owner.
- Reopening scalar ALU immediate materialization, large frame adjustment
  materialization, large stack-offset spelling, or unrelated frame-layout
  consistency unless generated evidence proves the same `va_start` destination
  publication fault.
- Fixing global initializer emission, later runtime mismatches, or later
  `stdarg` progression faults unless they become the next first bad fact after
  `va_start` destination publication is repaired.
- Changing semantic admission, unsupported classifications, expectations,
  CTest registration, runner behavior, timeout policy, or proof-log policy.

## Acceptance Criteria

- The unmaterialized `va_start` destination register is localized to concrete
  AArch64 backend records, homes, or printer code.
- `VaStart` lowering materializes or otherwise publishes a writable local
  `va_list` destination before emitting field stores.
- The repair is a general AArch64 `va_start` destination-address rule, not a
  `00204.c`, `stdarg`, `myprintf`, `x21`, or one-frame-slot shortcut.
- Focused backend coverage exercises the destination materialization contract.
- Prior-owner guardrails remain cleared under a fresh build and focused CTest
  proof.
- If `00204.c` advances to another first bad fact outside `va_start`
  destination materialization, the residual is classified into a separate
  owner.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00204.c`, `stdarg`, `myprintf`, `x21`, one local variable, one
  frame slot, one offset, or one emitted instruction sequence instead of
  repairing general AArch64 `va_start` destination publication;
- hides the failure by weakening expectations, unsupported classifications,
  runner behavior, timeout policy, proof-log policy, or CTest registration;
- claims progress while generated `va_start` field stores still use an
  unmaterialized destination register as their base;
- reopens aggregate `va_arg` helper lowering, F128 transport, HFA argument ABI,
  scalar ALU immediates, large stack/frame materialization, or global
  initializer emission without generated-code evidence tying that work to the
  `va_start` destination fault;
- adds coverage that only checks the c-testsuite representative while omitting
  focused backend coverage for the `va_start` destination materialization
  contract.
