# Current Packet

Status: Active
Source Idea Path: ideas/open/80_aarch64_dispatch_publication_owner_relocation.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Continue owner-by-owner relocation

## Just Finished

Completed Step 3 from `plan.md`: relocated the narrow store-retarget helper
family out of dispatch publication and into the AArch64 memory owner.
`retarget_pointer_store_value_to_materialized_address`,
`retarget_store_address_to_materialized_pointer`,
`retarget_pointer_store_value_to_emitted_scalar`, and
`retarget_store_local_value_to_emitted_scalar` are now declared from
`memory.hpp` and defined in `memory.cpp`; existing dispatch call sites keep the
same semantics through the memory owner surface.

Changed files:

- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.hpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/memory.hpp`
- `todo.md`

## Suggested Next

Classify and relocate the current-block join/publication hazard helper family
next if the supervisor keeps Step 3 active:
`block_entry_move_clobbers_current_join_publication`,
`prepared_value_home_reads_register_index`, and
`value_publication_may_read_register_index`.

## Watchouts

- `record_address_materialization_result` still lives in
  `dispatch_publication.cpp`; do not fold it into memory unless a later packet
  explicitly classifies address materialization ownership.
- The moved store-retarget helpers still consume existing prepared/emitted
  scalar authority from memory-owner utilities; no local re-derivation of
  publication authority was added.
- Root-level `test_before.log` and `test_baseline.log` were already present
  alongside the required `test_after.log`; this packet wrote only
  `test_after.log`.

## Proof

Ran the supervisor-selected proof command exactly:

```sh
cmake --build --preset default && bash -o pipefail -c "ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log"
```

Result: passed. The build completed and CTest reported `100% tests passed, 0
tests failed out of 169`.

Proof log: `test_after.log`
