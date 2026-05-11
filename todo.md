Status: Active
Source Idea Path: ideas/open/163_backend_semantic_bir_route_alignment.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Align Semantically Correct Stale BIR Snippets

# Current Packet

## Just Finished

Step 3 aligned the stale post-lowering semantic BIR snippet for test `100`, `backend_codegen_route_x86_64_global_aggregate_array_function_pointer_call_observe_semantic_bir`. The update is semantic-only: the regenerated BIR already contains the same global function-pointer load from `@gs` offset `8`, indirect call, and returned call result; only the temp names changed from `%t4/%t5` to `%t3/%t4` after the Step 4 lowering repair made semantic BIR available.

The Step 1 31-failure inventory remains available in committed `todo.md` at `86ba9f59b` (`[todo_only] Inventory backend semantic BIR route failures`); this packet keeps that inventory referenced rather than duplicating it over the current execution state.

## Suggested Next

Return to the supervisor for commit-boundary selection for the completed three-test lowering slice, or delegate the next narrow family from the committed Step 1 inventory if this slice is accepted.

## Watchouts

- The snippet update preserves the same required semantic operations and forbidden snippets; it does not mark the route unsupported, remove forbidden snippets, or broaden matching.
- Tests `96` and `161` had already passed the delegated route subset after the lowering repair; this packet touches only test `100`'s stale expected temp numbering.
- The current dirty lowering edits in `src/backend/bir/lir_to_bir/` are intentionally preserved for the supervisor's slice boundary.

## Proof

Ran the delegated proof into `test_after.log`; build succeeded and all three route tests passed:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_codegen_route_x86_64_(indirect_aggregate_param_return_pair|global_aggregate_array_function_pointer_call|aggregate_param_return_pair_fn_param)_observe_semantic_bir$'`
