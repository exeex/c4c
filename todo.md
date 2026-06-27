Status: Active
Source Idea Path: ideas/open/417_prepared_storage_layout_and_initializer_contracts.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Migrate One Global/Object-Data Consumer Path

# Current Packet

## Just Finished

Step 5 migrated the selected RV64 defined-global object-data path for
immediate scalar, immediate linear, and zero-fill globals. Prealloc now
publishes `PreparedGlobalObjectData` records with explicit labels, section
publication identity, emitted bytes or zero-fill, and object byte ranges.

`append_rv64_prepared_data_objects` now verifies those selected records with
`verify_prepared_selected_object_data_contract` and emits from prepared facts
instead of deriving labels, sections, initializer bytes, or BSS reservations
from raw `bir::Global` initializer shape. Missing prepared records,
contradictory emitted-byte facts, and complete unsupported initialized forms
fail closed through the selected object-data verifier classification.

Focused `backend_riscv_object_emission` coverage proves coherent mutable,
read-only, zero-fill, address-relocation, load, and store global object paths
over prepared object-data facts, plus fail-closed missing and incoherent
prepared object-data facts.

## Suggested Next

Execute Step 6 acceptance validation and close-readiness review for the active
storage/initializer contract runbook.

## Watchouts

- String constants remain on the existing object-data emission path; this
  packet intentionally migrated selected defined globals only.
- Extern global declarations without initializer storage are still skipped as
  declarations before selected object-data verification.
- Unsupported initialized global forms now require an explicit prepared
  unsupported marker and classify as `target_unsupported_but_coherent` rather
  than being rediscovered from raw initializer shape in the RV64 target.

## Proof

Ran exactly:
`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prealloc_prepared_contract_verifier$|backend_riscv_object_emission$|backend_rv64_runtime_global_|backend_rv64_runtime_defined_global_array|backend_codegen_route_riscv64_external_string_literal_strlen_direct_call$|backend_codegen_route_riscv64_external_strlen_runtime_link_policy$)' ) > test_after.log 2>&1`

Result: passed. `test_after.log` records 11/11 selected tests passing,
including `backend_prealloc_prepared_contract_verifier`,
`backend_riscv_object_emission`, the RV64 runtime global and defined global
array tests, and the two RV64 external strlen/string-literal route tests.
