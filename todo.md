Status: Active
Source Idea Path: ideas/open/73_aarch64_variadic_prepared_entry_plan_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Prepared Variadic Authority

# Current Packet

## Just Finished

Step 1 inventory completed for `src/backend/mir/aarch64/codegen/variadic.cpp` against prepared variadic authority.

Prepared facts available:
- `PreparedVariadicEntryPlanFunction` carries function identity, named parameter/register counts, register-save area, overflow area, va_list layout, helper resources, helper operand homes, and `missing_required_facts`.
- `populate_variadic_entry_plans` builds AAPCS64 entry facts in `src/backend/prealloc/variadic_entry_plans.cpp`: named GP/FP counts from formal ABI, required helper kinds from recognized variadic helper calls, AAPCS64 register-save/overflow/va_list layout facts, helper scratch resources, storage slot authority, and per-helper operand homes.
- `PreparedVariadicEntryHelperOperandHomes` carries `va_start` destination/address homes, scalar/aggregate `va_arg` source/result/payload homes, `va_copy` source/destination homes, and prepared scalar/aggregate access plans.
- Lookup surfaces already exist: `find_prepared_variadic_entry_plan` and `find_prepared_variadic_entry_helper_operand_homes` in `src/backend/prealloc/module.hpp`; `lower_call_instruction` in `src/backend/mir/aarch64/codegen/calls.cpp` already fetches both and threads them into `CallInstructionRecord`.

AArch64 local reconstruction/consumption sites:
- `require_prepared_variadic_entry_plan` consumes the prepared entry plan, but `missing_variadic_entry_facts` locally reconstructs entry-plan completeness by re-enumerating required prepared fields instead of relying only on prepared authority.
- `variadic_entry_helper_kind` in `variadic.cpp` duplicates the helper-kind callee classifier also present in prealloc; this is local helper classification policy used for dispatch.
- `variadic_helper_operand_homes_complete` locally reconstructs helper operand-home completeness, including scalar access-plan completeness. Aggregate completeness has a prepared helper, but scalar completeness does not.
- `make_variadic_va_start_record`, `make_variadic_scalar_va_arg_record`, `make_variadic_aggregate_va_arg_record`, and `make_variadic_va_copy_record` are target-local record construction from prepared entry/home facts. These are mostly consumption and validation gates, not ABI policy derivation.
- `complete_variadic_call_record`, `variadic_call_selection_status`, and `print_variadic_call` consume the structured records and keep target-local ABI emission/diagnostics local.
- `print_va_start_lowering_lines`, `append_va_start_register_save_area_lines`, scalar/aggregate `va_arg` printing helpers, and `va_copy` printing keep concrete AArch64 instruction selection, register naming, stack addressing, and copy chunking local.
- `emit_prepared_va_list_field_load_to_register` and the `prepared_va_list_field_address`/memory helpers are local target-address emission from prepared va_list field/home facts; they should stay target-local but should not grow new policy inference.

Missing prepared facts/blockers:
- No blocker for Step 2 entry-plan consumption was found: prepared entry plans expose the fields AArch64 record construction currently needs.
- Step 3 has one prepared-authority gap: there is no shared scalar counterpart to `has_complete_prepared_variadic_aggregate_va_arg_access_plan`, so `variadic_helper_operand_homes_complete` must currently reconstruct scalar access-plan completeness locally.
- Helper-kind classification is duplicated between prealloc and AArch64. A safe cleanup likely needs a shared helper-classification surface or an explicit decision to keep target dispatch classification local.

## Suggested Next

Step 2 packet: update `src/backend/mir/aarch64/codegen/variadic.cpp` so `require_prepared_variadic_entry_plan` treats `PreparedVariadicEntryPlanFunction::missing_required_facts` as the prepared entry-plan completeness authority and removes/reduces the local `missing_variadic_entry_facts` re-enumeration. Preserve target-local record-builder guard checks and AArch64 save-area/va_list emission.

## Watchouts

- Do not move AArch64 va_list layout spelling, save-area instruction selection, concrete stack/register addressing, aggregate copy chunking, or va_arg/va_copy record schemas into shared prealloc code.
- For Step 2, avoid deleting record-builder guards until proof shows `missing_required_facts` covers all required prepared fields; the first safe cut is the entry-plan diagnostic/completeness gate.
- For Step 3, add or expose a shared scalar access-plan completeness helper before removing the scalar completeness reconstruction in `variadic_helper_operand_homes_complete`.
- Do not weaken variadic expectations or add named-case shortcuts.

## Proof

Not run. Delegated proof was "No build required" for read-only inventory plus `todo.md` update; no proof logs were created.
