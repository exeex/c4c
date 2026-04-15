# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 2, broaden semantic call lowering beyond the current baseline

# Current Packet

## Just Finished
- added `indirect_variadic_pair_second.c` plus a matching backend route test to prove that aggregate-bearing indirect variadic calls also stay on the semantic-BIR route
- widened the nearby proof surface from the scalar indirect variadic case to a same-family aggregate-bearing function-pointer parameter shape without changing backend lowering ownership
- kept the slice on step-2 call lowering proof only; runtime-family behavior and unsupported-boundary wording did not change

## Suggested Next
- shift from adjacent indirect-variadic proof to another uncovered step-2 semantic call family only if it exposes a real route gap rather than another already-green neighbor
- if no honest step-2 gap appears under backend proof, request a route review or plan checkpoint instead of padding the variadic call cluster further

## Watchouts
- the aggregate-bearing proof still uses a function-pointer parameter wrapper because direct or optimizer-folded aliases can hide whether the indirect semantic call path actually owned the lowering
- this slice extends proof only; if the next packet also lands in `tests/backend/CMakeLists.txt`, acceptance should consider a fresh route checkpoint instead of stacking more narrow harness-only commits

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_codegen_route_x86_64_variadic_sum2_observe_semantic_bir|backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir|backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir|backend_codegen_route_x86_64_variadic_va_copy_accumulate_observe_semantic_bir|backend_codegen_route_x86_64_indirect_variadic_sum2_observe_semantic_bir|backend_codegen_route_x86_64_indirect_variadic_pair_second_observe_semantic_bir)$" > test_after.log 2>&1'`
- passed; the targeted variadic semantic-BIR route subset now proves both scalar and aggregate-bearing indirect variadic call shapes
- proof log preserved at `test_after.log`
