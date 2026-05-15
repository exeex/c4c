# AArch64 Variadic Machine Node Consumption

Status: Closed
Created: 2026-05-15
Closed: 2026-05-15

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

## Lifecycle Status

Step 1 inspection previously split out prerequisite idea
`ideas/closed/244_aarch64_variadic_prepared_storage_and_helper_authority.md`.
That prerequisite is now closed and supplied the prepared/shared facts needed
by this idea:

- `register_save_area.slot_id`
- `register_save_area.stack_offset_bytes`
- `overflow_area.base_slot_id`
- `overflow_area.base_stack_offset_bytes`
- helper scratch register counts and scratch stack bytes
- helper operand-home facts for `va_list` pointers, scalar destinations,
  aggregate destinations, and `va_copy` source/destination operands

This idea is reactivated for selected machine-node consumption. AArch64 target
lowering must consume those facts without reconstructing AAPCS64 frame,
layout, or scratch policy locally.

Parked on 2026-05-15 after Step 3 found a narrower prepared/shared prerequisite:
`helper_operand_homes.va_arg.scalar_access_plan`. Commit `a9b41b2a2` proved the
backend still passes while failing closed on that exact missing fact.

Reactivated on 2026-05-15 after
`ideas/closed/245_prepared_scalar_va_arg_access_plan.md` supplied the
prepared/shared scalar access-plan authority. Resume at scalar `va_arg`
consumption and consume `helper_operand_homes.va_arg.scalar_access_plan`
directly rather than reconstructing source classification, scalar
size/alignment, source coordinates, result-home relationship, or `va_list`
progression in AArch64 target lowering.

Parked on 2026-05-15 after Step 4 found the next prepared/shared prerequisite:
`helper_operand_homes.va_arg_aggregate.aggregate_access_plan`. Commit
`b426f024b` proved the backend still passes while failing closed on that exact
missing fact. Resume Step 4 only after
`ideas/open/246_prepared_aggregate_va_arg_access_plan.md` supplies aggregate
source selection, payload size/alignment, copy extent, destination/source
relationship, and `va_list` progression as shared prepared authority.

Reactivated on 2026-05-15 after
`ideas/closed/246_prepared_aggregate_va_arg_access_plan.md` supplied the
prepared/shared aggregate access-plan authority. Resume Step 4 at aggregate
`va_arg` selected machine-node consumption and consume
`helper_operand_homes.va_arg_aggregate.aggregate_access_plan` directly rather
than reconstructing source selection, payload size/alignment, source
coordinates, destination payload relationship, copy extent, or `va_list`
progression in AArch64 target lowering.

## Closure Notes

Closed after selected AArch64 variadic helper consumption landed for the
representative supported helper families:

- `va_start` selected machine-node consumption in commit `6009e54a9`
- scalar `va_arg` selected machine-node consumption in commit `b1a69adb6`
- aggregate `va_arg` selected machine-node consumption in commit `8cacc4348`
- `va_copy` selected machine-node consumption in commit `a52ccd25b`
- final lifecycle summary in commit `6d039823e`

The accepted route consumes prepared/shared homes, layout or access-plan facts,
and helper scratch resources for the selected helper paths. It does not
reconstruct AAPCS64 `va_list` layout, register-save or overflow offsets, named
argument counts, operand homes, scalar or aggregate access plans, or scratch
policy inside AArch64 selected-node lowering.

Prerequisite ideas 244, 245, and 246 supplied the missing prepared/shared
authority discovered during execution and are closed. No further blocker remains
for this idea's selected helper-consumption scope.

Close evidence: canonical backend proof in `test_before.log` records 139/139
passing, and the close-time regression guard comparison against that accepted
canonical proof passed with 139/139 before and after, no new failures. Recent
milestone full-suite baseline reviews were also accepted by the supervisor,
including 3167/3167.

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
