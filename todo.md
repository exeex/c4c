Status: Active
Source Idea Path: ideas/open/163_backend_semantic_bir_route_alignment.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Align Semantically Correct Stale BIR Snippets

# Current Packet

## Just Finished

Step 3 aligned the stale semantic BIR snippets for dynamic/member routes in tests `162` and `169` after the Step 4 lowering repair regenerated semantic BIR. Test `162` now requires the repaired pointer/member load from `addr %t1+4`, the direct `get_second` call, its regenerated return temp, and the surfaced local-array add/return temps. Test `169` now requires the regenerated local index load, sext, dynamic pointer select, direct `get_y` call, and return temp.

## Suggested Next

Return to the supervisor for commit-boundary selection for the Step 4 lowering repair plus this Step 3 stale-snippet alignment, or delegate the next narrow stale-snippet family from the active route inventory if this slice is accepted.

## Watchouts

- This packet only updates required snippet temp numbering for tests `162` and `169`; it preserves pointer/member loads, array offsets, dynamic pointer selection, direct calls, returns, and forbidden LLVM guards.
- The second half of dual test `162` surfaced after the first required-snippet fix; its local-array snippet was aligned to `%t10 = bir.add i32 %t6, %t9` and `bir.ret i32 %t10`.
- No route snippets were weakened, marked unsupported, broadened, or updated outside the delegated tests.

## Proof

Ran the delegated proof into `test_after.log`; build succeeded and both delegated route tests passed:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_codegen_route_x86_64_(nested_member_pointer_array|local_direct_dynamic_struct_array_call)_observe_semantic_bir$'`
