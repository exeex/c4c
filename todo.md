Status: Active
Source Idea Path: ideas/open/163_backend_semantic_bir_route_alignment.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Align Semantically Correct Stale BIR Snippets

# Current Packet

## Just Finished

Step 3 aligned the stale semantic BIR snippets for tests `140`, `143`, and `145`: `backend_codegen_route_x86_64_builtin_memcpy_nested_i32_array_field_observe_semantic_bir`, `backend_codegen_route_x86_64_builtin_memcpy_prefix_local_i32_array_to_pair_observe_semantic_bir`, and `backend_codegen_route_x86_64_builtin_memcpy_prefix_local_i32_subarray_to_nested_pair_field_observe_semantic_bir`. The updates are semantic-only: the required snippets now match regenerated semantic BIR temp numbering and return-value temps while preserving copy source slots, destination slots, byte offsets, copied values, and forbidden LLVM/memcpy guards.

## Suggested Next

Return to the supervisor for commit-boundary selection for the Step 4 lowering repair plus the Step 3 semantic snippet alignment slices, or delegate the next narrow stale-snippet family from the active route inventory if this slice is accepted.

## Watchouts

- The snippet update preserves required semantic copy operations and forbidden guards; it does not mark routes unsupported, remove forbidden snippets, or broaden matching.
- This packet touches only the delegated builtin memcpy nested/prefix semantic BIR snippets and leaves unrelated route snippets unchanged.

## Proof

Ran the delegated proof into `test_after.log`; build succeeded and all three route tests passed:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_codegen_route_x86_64_builtin_memcpy_(nested_i32_array_field|prefix_local_i32_array_to_pair|prefix_local_i32_subarray_to_nested_pair_field)_observe_semantic_bir$'`
