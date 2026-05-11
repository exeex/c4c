Status: Active
Source Idea Path: ideas/open/163_backend_semantic_bir_route_alignment.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Align Semantically Correct Stale BIR Snippets

# Current Packet

## Just Finished

Step 3 aligned the stale semantic BIR snippets for tests `93` and `102`: `backend_codegen_route_x86_64_byval_member_array_params_observe_semantic_bir` and `backend_codegen_route_x86_64_global_nested_aggregate_array_function_pointer_call_observe_semantic_bir`. The updates are semantic-only: regenerated BIR still emits byval aggregate parameter copies into locals and returns the loaded member values, with only return temp numbering changing to `%t3` for `param_member_array.c` and `%t4` for `nested_param_member_array.c`; the nested aggregate array function-pointer case still loads `@g` at offset `16`, calls indirectly through that pointer, and returns the call result, with temp numbering changing to `%t4`/`%t5`.

The Step 1 31-failure inventory remains available in committed `todo.md` at `86ba9f59b` (`[todo_only] Inventory backend semantic BIR route failures`); this packet keeps that inventory referenced rather than duplicating it over the current execution state.

## Suggested Next

Return to the supervisor for commit-boundary selection for this two-test semantic snippet alignment, or delegate the next narrow family from the committed Step 1 inventory if this slice is accepted.

## Watchouts

- The snippet update preserves the same required semantic operations and forbidden snippets; it does not mark the route unsupported, remove forbidden snippets, or broaden matching.
- This packet touches only the delegated byval member-array and nested aggregate array function-pointer semantic BIR snippets and leaves unrelated route snippets unchanged.

## Proof

Ran the delegated proof into `test_after.log`; build succeeded and both route tests passed:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_codegen_route_x86_64_(byval_member_array_params|global_nested_aggregate_array_function_pointer_call)_observe_semantic_bir$'`
