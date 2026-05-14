Status: Active
Source Idea Path: ideas/open/223_bir_mir_abstract_slot_contract_reject_string_names.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory String Placement Identity

# Current Packet

## Just Finished

Step 1 inventory completed for idea 223. Search proof found 812 string-placement hits across prealloc, AArch64 prepared handoff, and focused tests.

Authoritative string placement paths:
- `src/backend/prealloc/prealloc.hpp`: semantic register identity fields live in `PreparedSavedRegister::{register_name, occupied_register_names}`, `PreparedPhysicalRegisterAssignment::{register_name, occupied_register_names}`, `PreparedRegisterCandidateSpan::{register_name, occupied_register_names}`, `PreparedAllocationConstraint::{fixed_register_name, preferred_register_names, forbidden_register_names}`, `PreparedMoveResolution::{destination_register_name, destination_occupied_register_names}`, `PreparedAbiBinding::{destination_register_name, destination_occupied_register_names}`, `PreparedSpillReloadOp::{register_name, occupied_register_names}`, `PreparedCallArgumentPlan::{source_register_name, destination_register_name, destination_occupied_register_names}`, `PreparedCallResultPlan::{source_register_name, source_occupied_register_names, destination_register_name, destination_occupied_register_names}`, `PreparedClobberedRegister::{register_name, occupied_register_names}`, `PreparedCallPreservedValue::{register_name, occupied_register_names}`, `PreparedIndirectCalleePlan::register_name`, and `PreparedStoragePlanValue::{register_name, occupied_register_names}`.
- `src/backend/prealloc/target_register_profile.hpp` and `.cpp`: ABI and register-pool helpers publish spelling-derived `call_arg_destination_register_name`, `call_result_destination_register_name`, caller/callee saved register name lists, and indexed/numeric register-name sequences.
- `src/backend/prealloc/regalloc.cpp`: producers and equality/dedup paths materialize names into assignments, constraints, move destinations, ABI bindings, call result/return moves, active assignment overlap checks, spill/reload authority, and value homes.
- `src/backend/prealloc/prealloc.cpp`: prepared-frame/call/storage assembly copies those names into frame saves, preserved/clobbered records, call argument/result plans, move plans, spill/reload ops, and storage plans.
- `src/backend/mir/aarch64/module/module.hpp` and `.cpp`: prepared-to-AArch64 module records retain `std::string_view` spellings such as `physical_register`, `register_name`, `occupied_registers`, `destination_register`, `source_register`, and `target_destination_register`, then parse/copy prepared names during handoff.
- `src/backend/mir/aarch64/codegen/records.cpp`: scalar record construction uses prepared `home.register_name` and `storage.register_name` equality plus `convert_prepared_register(...)` to derive `RegisterOperand`, so prepared spelling is still semantic input to MIR records.
- `src/backend/mir/aarch64/abi/abi.hpp` and `.cpp`: `parse_aarch64_register_name(...)` and `convert_prepared_register(...)` are target-layer spelling conversion helpers; today they are also the bridge from prepared strings to `RegisterReference`.

Display/provenance-only or compatibility exceptions:
- `src/backend/prealloc/prepared_printer.cpp` prints `reg=`, `dest_reg=`, `source_reg=`, and occupied-register lists. This is display output and should remain only as derived text after structured placement exists.
- `src/backend/mir/aarch64/codegen/machine_printer.cpp` and `src/backend/mir/aarch64/abi::register_name(RegisterReference)` are final target spelling/printer routes and should remain target-layer spelling.
- `src/backend/mir/aarch64/MACHINE_INSTRUCTION_NODE_CONTRACT.md` documents that parsing prepared spellings is a transition seam; it is not an implementation authority.
- Test helper names such as `register_name_for_type(...)` are fixture conveniences, but many assertions still make those strings semantic and need conversion to abstract slot assertions in pre-target tests or `RegisterReference` assertions in AArch64 tests.

Affected focused tests:
- Pre-target prepared tests asserting string identity: `tests/backend/backend_prepare_liveness_test.cpp` and `tests/backend/backend_prepare_frame_stack_call_contract_test.cpp`, especially assigned registers, constraints, grouped spans, ABI arg/result destinations, spill/reload authority, indirect callees, and vector-width occupancy.
- AArch64 prepared tests consuming prepared spellings as input fixtures or asserting handoff identity: `tests/backend/backend_aarch64_prepared_register_conversion_test.cpp`, `backend_aarch64_prepared_operand_identity_test.cpp`, `backend_aarch64_prepared_module_identity_test.cpp`, `backend_aarch64_prepared_frame_control_test.cpp`, `backend_aarch64_prepared_scalar_alu_records_test.cpp`, `backend_aarch64_prepared_scalar_cast_records_test.cpp`, and `backend_aarch64_prepared_memory_operand_records_test.cpp`.

## Suggested Next

First implementation packet: Step 2, add structured prepared placement identity types without changing external target spelling behavior. Own `src/backend/prealloc/prealloc.hpp`, `src/backend/prealloc/prealloc.cpp`, `src/backend/prealloc/target_register_profile.hpp`, `src/backend/prealloc/target_register_profile.cpp`, and focused prealloc tests. Add an abstract placement model for register bank, slot pool, slot index, contiguous width, and spill slot placement, plus helpers that can derive current legacy spellings for compatibility during migration. Start with representative unit coverage in the prepared test bucket; do not rewrite all consumers in this packet.

Recommended narrow proof command: `bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -R "backend_prepare_(liveness|frame_stack_call_contract)" --output-on-failure'`

## Watchouts

- Do not let BIR/prepared placement facts choose or require target register spellings before target MIR mapping.
- Reject renaming string fields as progress unless the semantic contract moves to structured abstract slots.
- Do not enable unrelated AArch64 backend cases or weaken existing expectations.
- `PreparedRegisterBank` already exists, but the missing authority is slot pool/index/width identity; do not treat bank plus spelling as the new abstraction.
- `occupied_register_names` encodes both width and exact physical spelling. Preserve width semantics while moving physical identity to an abstract slot range.
- AArch64 `abi::parse_aarch64_register_name(...)` and `register_name(RegisterReference)` are legitimate target-layer spelling functions. The bug is prepared/pre-MIR depending on them for placement identity.
- `src/backend/prealloc/prepared_printer.cpp` can keep human-readable names only if they are derived display text, not the stored authority used by later lowering.

## Proof

Inspection proof only, per delegated packet. Ran:

`bash -lc 'set -o pipefail; { rg -n "register_name|occupied_register_names|source_register_name|source_occupied_register_names|destination_register_name|destination_occupied_register_names|fixed_register_name|preferred_register_names|forbidden_register_names|std::string_view.*register|register.*std::string_view" src/backend/prealloc src/backend/mir/aarch64 tests/backend/backend_prepare_* tests/backend/backend_aarch64_prepared_*; }' > test_after.log 2>&1`

Result: command completed successfully and wrote `test_after.log` with 812 matching lines. No build was required because this was an inventory-only packet with no implementation edits.
