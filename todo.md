Status: Active
Source Idea Path: ideas/open/42_riscv_prepared_edge_publication_aggregate_stack_source_policy.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate Aggregate Stack-Source Policy

# Current Packet

## Just Finished

Step 4 validated the RISC-V aggregate stack-source policy route as
closure-ready with a documented prepared-authority blocker.

Validation summary:

- Backend bucket proof passed 163/163.
- Focused route-quality searches found no aggregate stack-source scalar-load
  branch, no aggregate-specific load/copy support, and no local edge-copy
  rediscovery in `src/backend/mir/riscv/codegen/emit.cpp`.
- RISC-V edge-publication lowering still consumes
  `find_unique_indexed_prepared_edge_publication` as the shared edge-copy
  authority.
- The only stack-source register load path in `emit.cpp` remains the existing
  concrete-offset scalar path for source home sizes 4 and 8.
- The fail-closed route is closure-ready: aggregate-width stack-source support
  remains blocked on missing shared prepared authority for aggregate copy
  width, destination/lane mapping, alignment and partial-copy policy, ABI
  layout, and scratch ownership.

## Suggested Next

Plan owner can decide lifecycle closure for
`ideas/open/42_riscv_prepared_edge_publication_aggregate_stack_source_policy.md`
as a documented fail-closed prepared-authority blocker.

## Watchouts

Do not close this as positive aggregate load/copy support. The completed route
documents and proves the fail-closed policy because shared prepared authority
does not yet describe aggregate copy sequencing, lanes, alignment/partial-copy
policy, ABI layout, or scratch ownership.

## Proof

Passed:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log`

Result: 163/163 tests passed. Proof log: `test_after.log`.

Focused searches:

- `rg -n "aggregate|Aggregate|StackSlot|source_stack|find_unique_indexed_prepared_edge_publication|publications_by_edge_destination|predecessor|successor|lw |ld |0\(t6\)|t6" src/backend/mir/riscv/codegen/emit.cpp`
- `git diff -- src/backend/mir/riscv/codegen/emit.cpp tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`

Search result: `emit.cpp` has no diff in this route, no aggregate-specific
stack-source scalar load path, and no local rediscovery of edge-copy facts.
Closure-ready: yes, as a documented policy/prepared-authority blocker.
