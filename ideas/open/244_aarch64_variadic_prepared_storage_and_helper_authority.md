# AArch64 Variadic Prepared Storage And Helper Authority

Status: Open
Created: 2026-05-15

Parent Context: ideas/open/243_aarch64_variadic_machine_node_consumption.md

## Goal

Expose the prepared/shared storage, scratch-resource, and helper operand-home
facts required before AArch64 variadic helper machine-node consumption can
lower `va_start`, scalar `va_arg`, aggregate `va_arg`, or `va_copy`.

## Why This Exists

Step 1 of the AArch64 variadic machine-node consumption runbook found that
target lowering currently has helper classification and AAPCS64 layout facts,
but not the frame storage, overflow-base, helper-resource, or selected operand
authority needed to emit real machine nodes. Continuing directly would force
AArch64 target codegen to invent register-save-area placement, overflow base
placement, scratch policy, or concrete homes for helper pointer operands.

## In Scope

- Populate and validate `register_save_area.slot_id` and
  `register_save_area.stack_offset_bytes` for the prepared variadic entry
  register-save area.
- Populate and validate `overflow_area.base_slot_id` and
  `overflow_area.base_stack_offset_bytes`, including fail-closed missing-fact
  reporting when either cannot be supplied.
- Define and populate helper scratch-resource facts:
  `helper_resources.scratch_register_count` and
  `helper_resources.scratch_stack_bytes`.
- Add prepared/shared helper operand-home facts that map helper BIR operands to
  concrete machine-node operands for:
  `va_start` destination `va_list` pointers, scalar `va_arg` result homes,
  aggregate `va_arg` destination payload storage, aggregate/source `va_list`
  pointers, and `va_copy` source/destination `va_list` pointers.
- Keep AArch64 variadic helper dispatch fail-closed until these facts are
  present and complete.
- Add focused tests that prove missing facts are reported and populated facts
  are carried structurally rather than inferred by target lowering.

## Out Of Scope

- Do not add selected AArch64 machine-node lowering for `va_start`, `va_arg`,
  aggregate `va_arg`, or `va_copy`; that remains in idea 243 after this
  prerequisite lands.
- Do not reconstruct AAPCS64 `va_list` layout, register-save-area offsets,
  overflow-area offsets, named argument counts, or scratch policy inside
  AArch64 target lowering.
- Do not weaken existing unsupported diagnostics or mark variadic helpers as
  supported before selected machine-node consumption exists.
- Do not broaden into unrelated frame-slot placement, callee-save placement,
  preserved-value extent, memory load/store, scalar cast, aggregate copy, or
  printer rewrite work except where a narrow carrier is needed for the helper
  authority above.

## Acceptance Criteria

- Prepared variadic entry facts expose complete register-save-area storage:
  slot id and stack offset.
- Prepared variadic entry facts expose complete overflow-area base storage:
  base slot id and base stack offset, and missing base slot state is visible in
  missing-fact diagnostics rather than silently implied.
- Helper scratch register and stack-byte requirements are explicit prepared or
  shared facts for every recognized variadic helper family.
- Helper operand-home facts preserve concrete `va_list`, result, aggregate
  payload, and copy source/destination homes for selected-node consumers.
- Focused validation proves the prepared/shared facts are structurally present
  and that incomplete authority still fails closed.
- Idea 243 can be reactivated with AArch64 target lowering consuming these
  facts instead of deriving them locally.

## Reviewer Reject Signals

- The route adds fixture-name checks or helper-name shortcuts that only satisfy
  one known variadic testcase instead of populating general prepared/shared
  carriers.
- The route downgrades unsupported expectations, weakens fail-closed
  diagnostics, or claims progress by changing expected text while the same
  missing storage, scratch, or operand-home facts remain absent.
- The route implements `va_start`, `va_arg`, aggregate `va_arg`, or `va_copy`
  selected machine-node lowering directly in this prerequisite instead of
  producing the facts that idea 243 will consume.
- The route computes AAPCS64 frame placement, overflow offsets, named register
  counts, `va_list` layout, or scratch allocation inside AArch64 target
  lowering rather than exposing prepared/shared authority.
- The route renames carriers or moves diagnostics while preserving the old
  failure mode: no `register_save_area.slot_id`, no
  `register_save_area.stack_offset_bytes`, no `overflow_area.base_slot_id`, no
  `overflow_area.base_stack_offset_bytes`, no helper scratch counts/stack
  bytes, or no helper operand homes.
- The route broadens into unrelated backend machine-node families or global
  frame-allocation rewrites instead of the narrow prepared variadic authority
  needed to unblock idea 243.
