Status: Active
Source Idea Path: ideas/open/63_aarch64_shared_select_chain_dependency_authority.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Relocate AArch64 Select Materialization Ownership

# Current Packet

## Just Finished

Completed Step 5 ownership relocation for AArch64 select materialization.
Moved `select_chain_label`, `emit_select_chain_value_to_register`,
`make_select_chain_materialization_instruction`, and
`materialize_direct_global_select_chain_call_argument` out of broad
`dispatch_edge_copies` ownership into the precise
`select_materialization` owner. Direct callers now include the focused owner
for select-chain materialization APIs, while dispatch edge-copy code keeps only
edge-copy routing and uses the focused owner for its remaining target-local
label packaging. The moved code preserves target emission behavior and still
consumes prepared select-chain/call-argument dependency facts from shared
authority instead of rediscovering semantics locally.

## Suggested Next

Supervisor should route Step 6 proof/review for the ownership split and select-
chain coverage, including inspection for accidental semantic-discovery
fallbacks or broad-header coupling.

## Watchouts

This packet intentionally moved only target-local emission ownership. It did
not migrate indirect-callee semantic dependency authority or add new
select-chain discovery. The new owner still calls existing prepared
same-block select producer and prepared call-argument dependency queries; those
queries remain the semantic authority. The backend test that directly calls
`select_chain_label` now includes the precise header.

## Proof

Proof command run exactly:
`(cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: passed; `test_after.log` is the canonical proof log. `git diff --check`
also passed.
