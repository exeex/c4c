Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Broader Backend Checkpoint

# Current Packet

## Just Finished

Step 4 completed the broader backend checkpoint for the selected
stack-preservation prepared-lookup route. The delegated backend subset stayed
green after the prior Step 2 lookup-route and Step 3 helper-boundary changes.

Coverage observed in `test_after.log` includes the prepared lookup helper,
prepared BIR dumps, AArch64 call-boundary ownership and prepared-call-boundary
scalability, byval runtime/codegen coverage, publication plan coverage, selected
indirect call coverage, and the broader `backend_route` label bucket.

## Suggested Next

Have the supervisor decide whether this Step 4 checkpoint is ready to commit or
whether the active plan needs reviewer/plan-owner handling.

## Watchouts

- Do not work on `ideas/open/03_dispatch_responsibility_reduction.md`.
- Do not weaken tests or mark nearby preservation/publication cases
  unsupported to claim progress.
- The surviving prealloc helper is the durable prepared lookup API, not an
  AArch64 emission reconstruction boundary.
- The local `calls_moves.cpp` wrapper is intentionally emission-only because it
  performs operand-readiness validation for the sole move-emission use site.
- Do not fold in callee-saved republication/population or block-entry
  non-call-use reconstruction under this completed selected route.
- This packet made no implementation changes; it only refreshed the canonical
  backend proof log and recorded the Step 4 checkpoint.

## Proof

Passed.

Command:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: build succeeded; `ctest` reported 162/162 backend tests passed. Relevant
coverage includes `backend_prepared_lookup_helper`,
`backend_cli_dump_prepared_bir_is_prepared`,
`backend_cli_dump_prepared_bir_exposes_contract_sections`,
`backend_aarch64_call_boundary_owner`,
`backend_codegen_route_aarch64_prepared_call_boundary_scalability`,
`backend_runtime_byval_helper_payload_8_to_13`,
`backend_runtime_byval_helper_payload_9_to_14`,
`backend_runtime_byval_helper_mixed_hfa_payload`,
`backend_publication_plan_record`,
`backend_store_source_publication_plan`,
`backend_codegen_route_aarch64_global_function_pointer_table_selected_indirect_call`,
and the `backend_route` label bucket.
Proof log: `test_after.log`.
