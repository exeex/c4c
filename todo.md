Status: Active
Source Idea Path: ideas/open/163_backend_semantic_bir_route_alignment.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Align Semantically Correct Stale BIR Snippets

# Current Packet

## Just Finished

Step 3 aligned the stale semantic BIR snippets for tests `147`, `149`, `152`, `153`, and `154`: `backend_codegen_route_x86_64_builtin_memset_local_i32_array_observe_semantic_bir`, `backend_codegen_route_x86_64_builtin_memset_nonzero_local_i32_array_observe_semantic_bir`, `backend_codegen_route_x86_64_builtin_memset_nested_i32_array_field_observe_semantic_bir`, `backend_codegen_route_x86_64_builtin_memset_nonzero_nested_i32_array_field_observe_semantic_bir`, and `backend_codegen_route_x86_64_builtin_memset_nonzero_prefix_nested_i32_subarray_field_observe_semantic_bir`. The updates are semantic-only: the required snippets now match regenerated semantic BIR temp numbering, return-value temps, and prefix-route control-flow check load temps while preserving zero/nonzero memset stores, destination slots and offsets, result checks, and forbidden LLVM/memset guards.

## Suggested Next

Return to the supervisor for commit-boundary selection for the Step 3 semantic snippet alignment slices, or delegate the next narrow stale-snippet family from the active route inventory if this slice is accepted.

## Watchouts

- The snippet update preserves required zero/nonzero memset stores and forbidden guards; it does not mark routes unsupported, remove forbidden snippets, or broaden matching.
- The prefix nested subarray route still checks the unmodified surrounding fields and the memset-written `-1` subarray fields through regenerated load temps.
- This packet touches only the delegated builtin memset semantic BIR snippets and leaves unrelated route snippets unchanged.

## Proof

Ran the delegated proof into `test_after.log`; build succeeded and all five route tests passed:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_codegen_route_x86_64_builtin_memset_(local_i32_array|nonzero_local_i32_array|nested_i32_array_field|nonzero_nested_i32_array_field|nonzero_prefix_nested_i32_subarray_field)_observe_semantic_bir$'`
