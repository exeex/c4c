# Current Packet

Status: Active
Source Idea Path: ideas/open/80_aarch64_dispatch_publication_owner_relocation.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Continue owner-by-owner relocation

## Just Finished

Completed Step 3 from `plan.md`: relocated the fixed-formal store-local
publication helper family out of dispatch publication and into the AArch64
memory owner. `fixed_formal_scalar_store_mnemonic`,
`prepared_store_local_access`, `plan_fixed_formal_store_local_publication`,
and `lower_fixed_formal_store_local_publication` are now declared from
`memory.hpp` and defined in `memory.cpp`; dispatch call sites keep the same
semantics through the memory owner surface.

Changed files:

- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.hpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/memory.hpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- `todo.md`

The direct backend dispatch test needed only a compile-only include update to
see the moved memory-owner declaration. No test expectations or behavior were
changed.

## Suggested Next

Relocate the narrow store-retarget helper family next if the supervisor keeps
memory-owner relocation active: `retarget_pointer_store_value_to_materialized_address`,
`retarget_store_address_to_materialized_pointer`,
`retarget_pointer_store_value_to_emitted_scalar`, and
`retarget_store_local_value_to_emitted_scalar`.

## Watchouts

- `record_address_materialization_result` still lives in
  `dispatch_publication.cpp`; do not fold it into memory unless a later packet
  explicitly classifies address materialization ownership.
- Store retarget helpers still remain on the dispatch publication surface and
  are memory-adjacent, but this packet intentionally moved only the fixed-formal
  store-local publication family.
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
