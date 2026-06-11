Status: Active
Source Idea Path: ideas/open/175_prepared_function_lookups_aggregate_privacy.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Prove the selected move-bundle subset and decide next

# Current Packet

## Just Finished

Step 4 proved the supervisor-selected move-bundle subset after the Step 2/Step
3 projection boundary changes. The selected AArch64 call-boundary,
operand-resolution, and instruction-dispatch tests all pass with the current
standalone `prepare::PreparedMoveBundleLookups` path.

## Suggested Next

Hand lifecycle decision back to the supervisor/plan-owner: either close/retire
this runbook at the current move-bundle projection boundary or create/activate a
separate route for any next aggregate-field contraction.

## Watchouts

- Remaining aggregate exposure is intentionally out-of-scope for this Step 4
  proof: `memory_accesses`, `edge_publications`,
  `edge_publication_source_producers`, and `return_chains`.
- `FunctionLoweringContext::move_bundle_lookups` still points at the standalone
  traversal-local projection; keep its lifetime aligned with the full
  function-lowering loop.
- Do not migrate target value-location, storage-plan, register-allocation, call
  ABI, or move-record policy into BIR.
- Treat fixture aggregate-field assignments as test harness compatibility
  unless the supervisor explicitly delegates test cleanup.

## Proof

Supervisor-selected Step 4 move-bundle proof passed and was captured in
`test_after.log`:
`(cmake --build build --target backend_aarch64_call_boundary_owner_test backend_aarch64_operand_resolution_test backend_aarch64_instruction_dispatch_test -j && ctest --test-dir build -R '^(backend_aarch64_call_boundary_owner|backend_aarch64_operand_resolution|backend_aarch64_instruction_dispatch)$' --output-on-failure) > test_after.log 2>&1`.

Result: 3/3 tests passed.
