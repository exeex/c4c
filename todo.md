Status: Active
Source Idea Path: ideas/open/175_prepared_function_lookups_aggregate_privacy.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Prove the selected move-bundle subset and decide next

# Current Packet

## Just Finished

Step 3 removed the now-unneeded direct
`../../../prealloc/calls.hpp` include from AArch64 traversal after Step 2 moved
the selected move-bundle projection behind a standalone
`prepare::PreparedMoveBundleLookups` builder.

Production traversal still has no `prepared_lookups.move_bundles` read.
`PreparedFunctionLookups` remains available through the existing context path
for unselected aggregate consumers.

## Suggested Next

Execute Step 4 from `plan.md`: prove the selected move-bundle subset and decide
whether the next safe contraction should target another aggregate field or stop
with the current projection boundary.

## Watchouts

- Direct publication/source-producer, memory-access, and return-chain consumers
  remain outside the Step 3 contraction and still have live AArch64 aggregate
  reads.
- `FunctionLoweringContext::move_bundle_lookups` still points at the standalone
  traversal-local projection; keep its lifetime aligned with the full
  function-lowering loop.
- Do not migrate target value-location, storage-plan, register-allocation, call
  ABI, or move-record policy into BIR.
- Treat fixture aggregate-field assignments as test harness compatibility
  unless the supervisor explicitly delegates test cleanup.

## Proof

Supervisor-selected Step 3 proof passed and was captured in `test_after.log`:
`(cmake --build build --target backend_aarch64_call_boundary_owner_test backend_aarch64_operand_resolution_test backend_aarch64_instruction_dispatch_test -j && ctest --test-dir build -R '^(backend_aarch64_call_boundary_owner|backend_aarch64_operand_resolution|backend_aarch64_instruction_dispatch)$' --output-on-failure) > test_after.log 2>&1`.
