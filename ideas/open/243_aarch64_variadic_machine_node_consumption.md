# AArch64 Variadic Machine Node Consumption

Status: Open
Created: 2026-05-15

Parent Context: ideas/closed/232_aarch64_variadic_function_entry_carriers.md

## Goal

Consume prepared AArch64 variadic-entry facts in selected machine-node lowering
for `va_start`, scalar `va_arg`, aggregate `va_arg`, and `va_copy` without
rebuilding AAPCS64 ABI or frame-layout decisions in target codegen.

## Why This Exists

Idea 232 closed the prepared-carrier route: variadic callee entry facts are now
structured, dumped, covered by focused tests, and guarded fail-closed at the
AArch64 helper boundary. Actual machine-node consumption remains unsupported
because the helper lowering still needs complete storage and scratch facts,
including register-save-area slot/offset, overflow-area base slot/offset,
`va_list` layout storage, helper scratch counts, and machine-node operands for
materializing helper side effects.

## In Scope

- Consume `PreparedVariadicEntryPlanFunction` and related prepared facts
  directly from the AArch64 helper lowering path.
- Add selected machine-node records for the supported `va_start`, scalar
  `va_arg`, aggregate `va_arg`, and `va_copy` helper effects once complete
  prepared storage facts are available.
- Materialize `va_list` field initialization, register-save-area access,
  overflow-area progression, helper copies, and aggregate payload handling from
  structured prepared facts.
- Preserve explicit diagnostics for missing storage, offset, helper-resource,
  or scratch facts.
- Add focused tests that distinguish prepared-carrier presence from actual
  machine-node consumption and printer output.

## Out Of Scope

- Do not infer register-save slots, stack offsets, overflow bases, `va_list`
  layout, named GP/FP counts, or helper scratch policy inside AArch64 target
  lowering.
- Do not weaken existing fail-closed diagnostics or mark currently unsupported
  variadic helper cases as supported without selected machine-node evidence.
- Do not merge this route with unrelated global address, memory load/store,
  scalar cast, i128, binary128, atomic, intrinsic, inline-asm, callee-save
  slot-placement, or preserved-value extent work.
- Do not treat prepared dump coverage alone as proof of machine-node
  consumption.

## Acceptance Criteria

- AArch64 lowering consumes complete prepared variadic-entry facts for at least
  one representative `va_start` path and selected scalar, aggregate, and copy
  helper cases, or records a narrower lifecycle blocker with exact missing
  prepared facts.
- Supported helper paths produce structured selected machine nodes and printer
  output from prepared facts, not local ABI reconstruction.
- Missing or incomplete prepared facts still fail closed with explicit
  diagnostics.
- Backend validation covers the focused variadic helper routes plus a broader
  backend subset chosen by the supervisor.

## Lifecycle Blocker

Step 1 inspection found that this idea cannot proceed directly to selected
machine-node consumption without overfitting missing authority into AArch64
target lowering. The active prerequisite is now
`ideas/open/244_aarch64_variadic_prepared_storage_and_helper_authority.md`.

That prerequisite must supply the prepared/shared facts needed by this idea:

- `register_save_area.slot_id`
- `register_save_area.stack_offset_bytes`
- `overflow_area.base_slot_id`
- `overflow_area.base_stack_offset_bytes`
- helper scratch register counts and scratch stack bytes
- helper operand-home facts for `va_list` pointers, scalar destinations,
  aggregate destinations, and `va_copy` source/destination operands

This idea should be reactivated only after those facts exist as prepared/shared
authority and can be consumed without reconstructing AAPCS64 frame, layout, or
scratch policy in AArch64 machine-node lowering.

## Reviewer Reject Signals

- The route adds named-case matching for a particular variadic fixture instead
  of consuming general prepared helper records.
- The route computes AAPCS64 `va_list` layout, register-save offsets,
  overflow-area offsets, named argument counts, or scratch allocation in
  AArch64 target lowering instead of reading prepared facts.
- The route downgrades unsupported expectations, weakens diagnostics, or
  claims support through prepared dump text without selected machine-node
  consumption.
- The route renames helpers, moves diagnostics, or changes test expectations
  while preserving the old unsupported `va_start` / `va_arg` / `va_copy`
  behavior.
- The route broadens into unrelated AArch64 machine-node families or frame
  allocation policy instead of the variadic helper consumption boundary.
