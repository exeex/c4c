Status: Active
Source Idea Path: ideas/open/63_aarch64_shared_select_chain_dependency_authority.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Migrate Call-Argument Select-Chain Consumers

# Current Packet

## Just Finished

Completed Step 3 call-argument consumer migration.
`materialize_direct_global_select_chain_call_argument` now consumes the
`PreparedCallArgumentPlan` fact via
`find_prepared_call_argument_direct_global_select_chain_dependency` and no
longer calls `find_prepared_direct_global_select_chain_dependency` or builds
edge-publication source-producer lookups itself. The helper still owns AArch64
register spelling, scratch selection, instruction sequencing, target emission,
and scalar-state recording after the prepared fact is known. Missing or
mismatched call-argument facts fail closed through the prepared query and a
source-value-name match.

Focused AArch64 call-argument fixtures that bypass `populate_call_plans` now
carry explicit prepared call-argument direct-global select-chain facts so their
existing positive behavior expectations remain covered without falling back to
local traversal.

## Suggested Next

Supervisor should delegate Step 4 to migrate ALU and value-materialization
select consumers to the shared/prepared dependency facts without changing
target-local materialization behavior.

## Watchouts

This packet intentionally left indirect-callee, ALU/value-materialization, and
`dispatch_producers.cpp` raw fallback consumers untouched. The remaining
`find_prepared_direct_global_select_chain_dependency` call in `calls.cpp` is
the indirect-callee path and belongs to a later packet, not this
call-argument migration.

## Proof

Proof command run exactly:
`(cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^backend_(aarch64_instruction_dispatch|aarch64_call_boundary_owner|prepare_frame_stack_call_contract|prepared_lookup_helper)$') > test_after.log 2>&1`

Result: passed; `test_after.log` is the canonical proof log. `git diff --check`
also passed.
