Status: Active
Source Idea Path: ideas/open/175_prepared_function_lookups_aggregate_privacy.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory aggregate field consumers and choose the first group

# Current Packet

## Just Finished

Step 1 from `plan.md`: inventoried direct `PreparedFunctionLookups` aggregate
field reads and selected `call_plans` as the first contraction group.

Inventory evidence commands:

- `rg -n "prepared_lookups(->|\\.)(call_plans|address_materializations|memory_accesses|move_bundles|value_homes|edge_publications|edge_publication_source_producers|return_chains)" src tests`
- `rg -n "prepared_lookups->(call_plans|address_materializations|memory_accesses|move_bundles|value_homes|edge_publications|edge_publication_source_producers|return_chains)" src`
- `ctest --test-dir build -N | rg "aarch64.*(scalar|alu|memory|dispatch)|prepared.*lookup"`

Direct aggregate field read inventory:

- `call_plans`: target-local route boundary. Production direct read is
  `src/backend/mir/aarch64/codegen/traversal.cpp:79`, assigning
  `FunctionLoweringContext::call_plan_lookups`. Fixture-style direct reads also
  exist in `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp`,
  `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`, and
  `tests/backend/mir/backend_aarch64_current_block_join_routing_test.cpp`.
- `address_materializations`: target-local address-layout boundary. Production
  direct read is `src/backend/mir/aarch64/codegen/traversal.cpp:80-81`,
  assigning `FunctionLoweringContext::address_materialization_lookups`. The same
  three AArch64 fixture files assign this pointer from local aggregate fixtures.
- `memory_accesses`: target-local prepared-addressing boundary. Production
  direct reads are `src/backend/mir/aarch64/codegen/alu.cpp:1037` and
  `src/backend/mir/aarch64/codegen/alu.cpp:1106`; both pass
  `PreparedMemoryAccessLookups` to typed prealloc helpers.
- `move_bundles`: target-local value-location boundary. Production direct read
  is `src/backend/mir/aarch64/codegen/traversal.cpp:82`, assigning
  `FunctionLoweringContext::move_bundle_lookups`. The same three AArch64
  fixture files assign this pointer from local aggregate fixtures.
- `value_homes`: target-local value-location boundary. Production direct read is
  `src/backend/mir/aarch64/codegen/traversal.cpp:83`, assigning
  `FunctionLoweringContext::value_home_lookups`. The same three AArch64 fixture
  files assign this pointer from local aggregate fixtures.
- `edge_publications`: blocked for the first contraction group. Production
  direct reads span `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`,
  `src/backend/mir/aarch64/codegen/memory.cpp`, and
  `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`; tests also mutate
  and pass this aggregate field in
  `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`.
- `edge_publication_source_producers`: route-owned publication-source boundary
  but too broad for the first group. Production direct reads span
  `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`,
  `src/backend/mir/aarch64/codegen/comparison.cpp`,
  `src/backend/mir/aarch64/codegen/alu.cpp`,
  `src/backend/mir/aarch64/codegen/memory.cpp`,
  `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`, and
  `src/backend/mir/aarch64/codegen/calls.cpp`; tests also mutate this field in
  `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`.
- `return_chains`: return-chain group. Production direct reads are
  `src/backend/mir/aarch64/codegen/alu.cpp:1370` and
  `src/backend/mir/aarch64/codegen/alu.cpp:4208`. Explicitly rejected for this
  runbook; idea 176 owns return-chain owner/schema analysis.

Selected first group:

- Field group: `call_plans`.
- Owner boundary: existing `FunctionLoweringContext::call_plan_lookups` pointer
  plus `PreparedCallPlanLookups` helpers from `src/backend/prealloc/calls.hpp`;
  downstream AArch64 call lowering already consumes this narrow pointer instead
  of `PreparedFunctionLookups::call_plans`.
- Direct production consumer file: `src/backend/mir/aarch64/codegen/traversal.cpp`.
- Rationale: it is the smallest safe production group, with one aggregate field
  read and an existing narrow lookup boundary. It does not touch
  `return_chains`, does not require a renamed aggregate, and does not cross the
  publication-source or memory-access routes.

## Suggested Next

Implement Step 2 for the selected `call_plans` group: keep
`FunctionLoweringContext::call_plan_lookups` as the owner boundary, remove the
direct `prepared_lookups.call_plans` read in
`src/backend/mir/aarch64/codegen/traversal.cpp`, and leave the aggregate
available for all unselected fields.

## Watchouts

- Do not hide `PreparedFunctionLookups` wholesale.
- Do not migrate or contract `return_chains` through this runbook.
- Do not create a renamed aggregate or generic BIR lowering-plan facade.
- Do not edit `ideas/open/175_prepared_function_lookups_aggregate_privacy.md`
  for routine execution notes.
- Do not select `return_chains`; that group remains rejected for this runbook.
- Treat the AArch64 fixture aggregate-field assignments as test harness
  compatibility unless the supervisor explicitly delegates test cleanup.

## Proof

No build proof required for this inventory-only packet. Proof was the scan
commands recorded above. Suggested exact proof command for the next
implementation packet:

```sh
cmake --build build --target backend_aarch64_call_boundary_owner_test backend_aarch64_instruction_dispatch_test -j && ctest --test-dir build -R '^(backend_aarch64_call_boundary_owner|backend_aarch64_instruction_dispatch)$' --output-on-failure
```
