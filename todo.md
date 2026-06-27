# Current Packet

Status: Active
Source Idea Path: ideas/open/416_prepared_helper_operand_home_contracts.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Inventory Current Variadic Helper Payloads

## Just Finished

Step 2 inventoried the current variadic helper optional-bag contract before
changing payload shapes.

Concrete implementation files for the first helper-contract packet:
`src/backend/prealloc/variadic.hpp`,
`src/backend/prealloc/variadic_entry_plans.cpp`,
`src/backend/prealloc/module.hpp`,
`src/backend/prealloc/prepared_contract_verifier.hpp`, and
`src/backend/prealloc/prepared_contract_verifier.cpp`. Keep AArch64/RV64 target
consumer files read-only for Step 3 unless compile adaptation forces a narrow
accessor update.

Current `PreparedVariadicEntryHelperOperandHomes` optional-bag fields:
`helper`, `block_index`, `instruction_index`, `destination_va_list`,
`destination_va_list_address`, `source_va_list`, `scalar_result`,
`aggregate_destination_payload`, `scalar_access_plan`, and
`aggregate_access_plan`.

Helper-specific payload needs:
- `va_start`: `destination_va_list` and `destination_va_list_address`.
- Scalar `va_arg`: `source_va_list`, `scalar_result`, and complete
  `scalar_access_plan` (`source_class`, payload type/size/align, result home,
  source/progression/overflow fields, field offsets, slot size, and strides).
- Aggregate `va_arg`: `source_va_list`, `aggregate_destination_payload`, and
  complete `aggregate_access_plan` (source class, call location, payload
  size/align, destination payload home, optional RV64 `payload_write_address`,
  source/progression/copy fields and sizes, overflow stride, and optional
  register-save lane homes).
- `va_copy`: `destination_va_list` and `source_va_list`; AArch64 record
  construction also consumes `va_list_layout` fields and helper scratch facts.

Producer/verifier/consumer surfaces:
- Producers live in `src/backend/prealloc/variadic_entry_plans.cpp`:
  `populate_aapcs64_variadic_entry_helper_operand_home_authority(...)` and
  `populate_rv64_variadic_entry_va_start_operand_home_authority(...)` fill the
  optional bag, call the access-plan builders, and publish missing facts.
- Lookup compatibility is
  `find_prepared_variadic_entry_helper_operand_homes(...)` in
  `src/backend/prealloc/module.hpp`, keyed by block and instruction index.
- Completeness helpers live in `src/backend/prealloc/variadic.hpp`:
  `has_complete_prepared_variadic_va_start_operand_homes`,
  `has_complete_prepared_variadic_scalar_va_arg_access_plan`,
  `has_complete_prepared_variadic_aggregate_va_arg_access_plan`,
  `has_complete_prepared_variadic_va_copy_operand_homes`, and the aggregate
  dispatcher.
- Verifier surface is
  `verify_prepared_variadic_entry_helper_operand_homes_contract(...)`, which
  returns `producer_missing` for absent homes and `producer_incoherent` for
  wrong-helper or incomplete homes, with detail
  `prepared variadic helper operand-home/access-plan facts are incomplete`.
- AArch64 consumers are `src/backend/mir/aarch64/codegen/calls.cpp` for lookup
  and fail-closed diagnostic attachment, plus
  `src/backend/mir/aarch64/codegen/variadic.cpp` for record construction,
  machine-node status, and printable lowering.
- RV64 consumers are `src/backend/mir/riscv/codegen/object_emission.cpp`
  diagnostics/fragments for `va_start` and aggregate `va_arg`, plus the helper
  admission path that rejects scalar `va_arg` and `va_copy` as unsupported.
- Printer/oracle compatibility is
  `src/backend/prealloc/prepared_printer/variadic.cpp`, which renders
  `helper_operand` rows and scalar/aggregate access-plan details.

Existing helper-specific missing facts and diagnostics already include
`helper_operand_homes.va_start.destination_va_list`,
`helper_operand_homes.va_start.destination_va_list_address`,
`helper_operand_homes.va_arg.source_va_list`,
`helper_operand_homes.va_arg.scalar_result`,
`helper_operand_homes.va_arg.scalar_access_plan`,
`target_abi.va_arg.scalar_payload_abi`,
`helper_operand_homes.va_arg_aggregate.source_va_list`,
`helper_operand_homes.va_arg_aggregate.aggregate_destination_payload`,
`helper_operand_homes.va_arg_aggregate.aggregate_access_plan`,
`helper_operand_homes.va_arg_aggregate.payload_write_address`,
`target_abi.va_arg_aggregate.payload_abi`,
`helper_operand_homes.va_copy.destination_va_list`, and
`helper_operand_homes.va_copy.source_va_list`. AArch64 already names missing
scalar/aggregate access-plan facts and complete `va_copy` source/destination
homes in target diagnostics; RV64 already names missing or incomplete prepared
helper operand homes and the supported `va_start`/aggregate `va_arg` materialized
requirements.

## Suggested Next

Execute Step 3 from `plan.md` as the first prepared-side packet: introduce typed
payload structures and compatibility accessors in the prealloc contract layer,
starting with `va_start`, while keeping the old optional-bag fields available to
existing AArch64/RV64 consumers until their migration steps.

## Watchouts

- Do not edit the source idea for routine execution notes.
- Do not infer helper operand homes in target lowering.
- Do not weaken tests or expectations to claim progress.
- First helper packet boundary: add typed `va_start` payload representation and
  accessors without yet deleting optional-bag fields or migrating target
  lowering; preserve block/instruction lookup identity and current verifier
  owner classifications.
- Existing RV64 producer behavior only pushes complete `va_start` homes, but
  pushes scalar `va_arg`, aggregate `va_arg`, and `va_copy` rows in more
  incomplete states so diagnostics can name helper-specific missing facts.
- Aggregate `va_arg` has an RV64-only `payload_write_address` requirement that
  is not part of the current generic completeness predicate.

## Proof

Inventory-only proof; no build required and no `test_after.log` was written
because this packet excluded test logs. Inspected the prepared data/producers,
verifier, AArch64 consumers, RV64 consumers, printer/oracle surface, and CTest
names with `rg`, file inspection, and `ctest --test-dir build -N`.

Focused Step 3 proof command:
`cmake --build build --target backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_prealloc_prepared_contract_verifier_test && ctest --test-dir build --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_prealloc_prepared_contract_verifier)$' | tee test_after.log`
