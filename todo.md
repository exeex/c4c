Status: Active
Source Idea Path: ideas/open/65_aarch64_target_owner_relocation.md
Source Plan Path: plan.md
Current Step ID: Step 6
Current Step Title: Prove Target-Owner Preservation

# Current Packet

## Just Finished

Step 6 completed: target-owner preservation and closure-readiness evidence was
collected for idea 65 without implementation or test edits.

Proof evidence:
- Full backend bucket proof passed with 169/169 tests green.
- `test_after.log` contains the green build and CTest output.
- `git diff --check` passed after proof.

Route-quality scans:
- Relocated global helpers `find_load_global_target` and
  `load_global_symbol_label` are declared/defined in `globals.*`; no broad
  `dispatch_producers.*` declaration remains.
- Relocated memory/address helpers `register_indirect_address`,
  `fixed_slots_use_frame_pointer`, `frame_slot_address`,
  `scalar_load_mnemonic`, `dispatch_publication_scalar_type_size_bytes`,
  `scalar_load_mnemonic_for_width`, and `scalar_store_mnemonic` are
  declared/defined in `memory.*`; broad dispatch-family headers do not declare
  them.
- Relocated register alias helpers `registers_alias` and
  `register_operands_share_physical_register` are declared/defined in
  `alu.*`; broad `dispatch_publication.*` uses are call sites only.
- `dispatch.cpp` did not accumulate relocated target helper logic. Its scan
  hits are route-level call/hook wiring for `registers_alias` and
  `fixed_slots_use_frame_pointer`.
- No forwarding-only wrappers were added for the relocated helper names.

## Suggested Next

The active plan appears ready for plan-owner closure review. Suggested next
packet: plan owner should compare the Step 6 evidence against idea 65
acceptance criteria, decide whether the source idea can close, and handle
lifecycle closure or any final route notes.

## Watchouts

No `publication.*` owner existed in this checkout, so remaining
dispatch-publication declarations are justified rather than force-moved into a
new source during surface cleanup. Residual risk is limited to whether the
supervisor wants broader-than-backend validation before closure; within the
delegated backend bucket and route-quality scans, no forwarding-only wrapper,
dispatch.cpp accumulation, or local behavior-change signal is visible.

The global relocation helper in `dispatch_publication.*` and the private
global-load publication helper in `dispatch_value_materialization.cpp` remain
deferred.

## Proof

Proof passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

`test_after.log` reports 169/169 backend tests passed. `git diff --check`
passed. Scan commands run:
- `rg -n "find_load_global_target|load_global_symbol_label|register_indirect_address|fixed_slots_use_frame_pointer|frame_slot_address|scalar_load_mnemonic|dispatch_publication_scalar_type_size_bytes|scalar_load_mnemonic_for_width|scalar_store_mnemonic|registers_alias|register_operands_share_physical_register" src/backend/mir/aarch64/codegen/dispatch_producers.* src/backend/mir/aarch64/codegen/dispatch_publication.* src/backend/mir/aarch64/codegen/dispatch_value_materialization.* src/backend/mir/aarch64/codegen/globals.* src/backend/mir/aarch64/codegen/memory.* src/backend/mir/aarch64/codegen/alu.*`
- `rg -n "find_load_global_target|load_global_symbol_label|register_indirect_address|fixed_slots_use_frame_pointer|frame_slot_address|scalar_load_mnemonic|dispatch_publication_scalar_type_size_bytes|scalar_load_mnemonic_for_width|scalar_store_mnemonic|registers_alias|register_operands_share_physical_register|emit_prepared_global_symbol_load_to_register|emit_prepared_global_load_to_register" src/backend/mir/aarch64/codegen/dispatch.cpp`
