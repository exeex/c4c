Status: Active
Source Idea Path: ideas/open/01_shared_call_plan_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit the Existing Boundary-Move Authority

# Current Packet

## Just Finished

Step 1 audited prepared call-plan authority against the AArch64 call-boundary
move and preservation surfaces.

Chosen first migration family: callee-saved preservation boundary effects.
Shared prepared facts already expose this as
`PreparedCallBoundaryEffectPlan` entries with
`effect_kind = PreservationHomePopulation` before the call and
`effect_kind = PreservationRepublication` after the call, populated from
`PreparedCallPlan::preserved_values`.

Prepared facts to consume:

- `PreparedCallPlan::preserved_values`
- `PreparedCallPreservedValue::{value_id,value_name,route,register_name,register_bank,contiguous_width,occupied_register_names,callee_saved_save_index,slot_id,stack_offset_bytes,stack_size_bytes,stack_align_bytes,register_placement,spill_slot_placement}`
- `plan_prepared_call_boundary_effects()` output, especially
  `PreparedCallBoundaryEffectPlan::{effect_kind,phase,source,destination,preservation_route,reason}`

AArch64 helpers that should stop owning this decision family:

- `lower_before_call_moves()` should stop directly iterating
  `call_plan.preserved_values` to decide preservation home population.
- `lower_after_call_moves()` should stop directly iterating
  `call_plan.preserved_values` to decide preservation republication.
- `make_callee_saved_preservation_home_population()` and
  `make_callee_saved_preservation_home_republication()` should become
  emission helpers driven by a prepared boundary-effect record rather than the
  local decision entry points for whether a preservation effect exists.

## Suggested Next

Implement the first migration packet for callee-saved preservation boundary
effects: route `lower_before_call_moves()` and `lower_after_call_moves()`
through `plan_prepared_call_boundary_effects()` preservation entries, pass the
selected prepared effect into the preservation emission helper, and keep the
helper responsible only for AArch64 operand conversion and machine-instruction
construction.

## Watchouts

- Do not start AArch64 calls file consolidation before shared authority is
  proven.
- Do not hard-code AArch64-only facts into the shared planner.
- Do not weaken tests or add named-case matching as proof of progress.
- The prepared effect endpoint currently carries the preservation storage
  facts but not a pointer/reference back to the exact
  `PreparedCallPreservedValue`; the next packet should either map effects back
  by `value_id` plus `preservation_route` or add a minimal shared prepared fact
  before changing AArch64 emission.
- AArch64 still has local liveness/use gates for republication
  (`preserved_value_has_later_non_call_use` and
  `preserved_value_has_block_entry_non_call_use`). Do not silently move those
  decisions unless the prepared layer exposes equivalent facts.

## Proof

`printf 'Audit-only Step 1; no code changed, no build/test run required.\n' > test_after.log`

Proof is sufficient for this audit-only packet because no implementation or
test files changed. Proof log: `test_after.log`.
