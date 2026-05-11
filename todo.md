Status: Active
Source Idea Path: ideas/open/163_backend_semantic_bir_route_alignment.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Align Semantically Correct Stale BIR Snippets

# Current Packet

## Just Finished

Step 3 aligned stale semantic BIR snippets for residual dynamic/member tests `163` through `168` after regenerated semantic BIR changed only temp numbering. The snippets now match the regenerated temps while preserving pointer/member loads, packed offsets, dynamic select chains, dynamic store fanout, direct calls/returns, dual-test local-array add/return snippets, and the existing forbidden LLVM guards.

## Suggested Next

Return to the supervisor for commit-boundary selection for the Step 3 stale-snippet alignment across tests `163` through `168`, or delegate the next narrow stale-snippet family from the active route inventory if this slice is accepted.

## Watchouts

- This packet only updates required snippets for tests `163`, `164`, `165`, `166`, `167`, and `168`; it does not touch implementation files, forbidden snippets, route matching, unsupported status, or unrelated route snippets.
- Dual tests `164`, `165`, `166`, and `167` needed their second snippets aligned where the regenerated local-array or neighbor BIR surfaced after the first snippet refresh.
- Test `168` still requires the packed member offsets at `%lv.p.1`, `%lv.p.5`, and `%lv.p.9` and now also names the regenerated `%t15` dynamic select before return.

## Proof

Ran the delegated proof into `test_after.log`; build succeeded and all six delegated route tests passed:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_codegen_route_x86_64_(nested_pointer_param_dynamic_member_array_load|local_dynamic_member_array|local_dynamic_member_array_store|local_direct_dynamic_member_array_store|local_direct_dynamic_member_array_load|packed_local_member_offsets)_observe_semantic_bir$'`
