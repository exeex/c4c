Status: Active
Source Idea Path: ideas/open/175_prepared_function_lookups_aggregate_privacy.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Contract selected direct dependency exposure

# Current Packet

## Just Finished

Step 2 added a standalone `prepare::PreparedMoveBundleLookups` builder path
that produces the base move-bundle indexes and publishes the same after-call
result lane binding records as the aggregate `PreparedFunctionLookups` path.

AArch64 traversal now builds a local `PreparedMoveBundleLookups` projection and
sets `FunctionLoweringContext::move_bundle_lookups` from that local value.
Production traversal no longer reads `prepared_lookups.move_bundles`; the
aggregate remains available for unselected field consumers.

## Suggested Next

Execute Step 3 from `plan.md`: remove only the direct aggregate dependency or
include exposure made unnecessary by the standalone move-bundle projection,
while keeping `PreparedFunctionLookups` available for unselected aggregate
field consumers.

## Watchouts

- Direct publication/source-producer, memory-access, and return-chain consumers
  remain outside this packet and still have live AArch64 aggregate reads.
- `FunctionLoweringContext::move_bundle_lookups` now points at the standalone
  traversal-local projection; keep its lifetime aligned with the full
  function-lowering loop.
- Do not migrate target value-location, storage-plan, register-allocation, call
  ABI, or move-record policy into BIR.
- Treat fixture aggregate-field assignments as test harness compatibility
  unless the supervisor explicitly delegates test cleanup.

## Proof

Supervisor-selected proof passed and was captured in `test_after.log`:
`(cmake --build build --target backend_aarch64_call_boundary_owner_test backend_aarch64_operand_resolution_test backend_aarch64_instruction_dispatch_test -j && ctest --test-dir build -R '^(backend_aarch64_call_boundary_owner|backend_aarch64_operand_resolution|backend_aarch64_instruction_dispatch)$' --output-on-failure) > test_after.log 2>&1`.
