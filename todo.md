Status: Active
Source Idea Path: ideas/open/163_backend_semantic_bir_route_alignment.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Align Semantically Correct Stale BIR Snippets

# Current Packet

## Just Finished

Step 3 aligned the stale semantic BIR snippets for tests `105`, `106`, `107`, and `108`: `backend_codegen_route_x86_64_global_struct_array_read_observe_semantic_bir`, `backend_codegen_route_x86_64_global_struct_array_store_observe_semantic_bir`, `backend_codegen_route_x86_64_nested_global_struct_array_read_observe_semantic_bir`, and `backend_codegen_route_x86_64_nested_global_struct_array_store_observe_semantic_bir`. The updates are semantic-only: regenerated BIR preserves the same global names (`@pairs`, `@s`), offsets (`12`, `8`, `8`, `4`), store/load operations, and returned load values; only temp numbering changed to `%t3`, `%t6`, `%t4`, and `%t8`.

The Step 1 31-failure inventory remains available in committed `todo.md` at `86ba9f59b` (`[todo_only] Inventory backend semantic BIR route failures`); this packet keeps that inventory referenced rather than duplicating it over the current execution state.

## Suggested Next

Return to the supervisor for commit-boundary selection for this four-test semantic snippet alignment, or delegate the next narrow family from the committed Step 1 inventory if this slice is accepted.

## Watchouts

- The snippet update preserves the same required semantic operations and forbidden snippets; it does not mark the route unsupported, remove forbidden snippets, or broaden matching.
- This packet touches only the four delegated global struct/array read/write semantic BIR snippets and leaves unrelated route snippets unchanged.

## Proof

Ran the delegated proof into `test_after.log`; build succeeded and all four route tests passed:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_codegen_route_x86_64_(global_struct_array_read|global_struct_array_store|nested_global_struct_array_read|nested_global_struct_array_store)_observe_semantic_bir$'`
