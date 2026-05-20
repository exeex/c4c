Status: Active
Source Idea Path: ideas/open/347_aarch64_local_conversion_store_load_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Focused Backend Coverage

# Current Packet

## Just Finished

Step 3 added focused AArch64 backend coverage in
`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` for local
scalar/FP conversion store publication. The new fixture lowers a same-block
`cast -> storelocal` route for both FP-to-integer and integer-to-FP conversion
sides, verifies the store-owned publication precedes the selected local store,
and checks the store consumes the converted register without pinning scratch
register names or stack offsets.

## Suggested Next

Supervisor should accept the Step 3 coverage slice with the Step 2 dispatch
repair, then choose the next packet from the active plan. A coherent next
packet is to run the supervisor-selected broader acceptance proof or move to
plan review/closure if the active runbook has no remaining implementation
work.

## Watchouts

- The coverage intentionally lives in the instruction-dispatch test surface,
  which already owns same-block scalar-cast and local-store publication
  behavior.
- The assertions compare publication/store order and converted-register
  consumption, not exact scratch registers or frame offsets.
- The test fixture allows the ordinary cast node to lower before the store;
  the pinned contract is that the store path still emits its own publication
  immediately before the local home store.

## Proof

Ran the delegated proof command exactly:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_(instruction_dispatch|scalar_cast_records|prepared_scalar_cast_records|memory_operand_contract)$' | tee test_after.log`.
The build was up to date after the test recompile, and all four delegated
backend tests passed. Proof log: `test_after.log`.
