# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 2, broaden semantic call lowering beyond the current baseline

# Current Packet

## Just Finished
- widened indirect semantic call parsing so variadic function-pointer calls keep their actual argument metadata instead of escaping the semantic-BIR route
- added `indirect_variadic_sum2.c` to lock the repaired x86_64 semantic-BIR route for an indirect variadic call through a function-pointer parameter
- kept the slice on step-2 call lowering only; runtime-family behavior and unsupported-boundary wording did not change

## Suggested Next
- stay on step 2 and probe the next real indirect-call gap near this fix, especially a nearby variadic or aggregate-bearing function-pointer shape that still escapes semantic BIR without relying on optimizer-folded direct aliases
- if the indirect-call route now looks dense enough, shift the next packet to another uncovered semantic call family rather than adding more adjacent proof

## Watchouts
- the repaired case is proven through a function-pointer parameter wrapper because a local alias to a known symbol can fold back to a direct call in semantic BIR output
- `tests/backend/CMakeLists.txt` is still test-harness surface, so acceptance should keep a broader backend checkpoint in mind if several more step-2 slices land without one

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_codegen_route_x86_64_variadic_sum2_observe_semantic_bir|backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir|backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir|backend_codegen_route_x86_64_variadic_va_copy_accumulate_observe_semantic_bir|backend_codegen_route_x86_64_indirect_variadic_sum2_observe_semantic_bir)$" > test_after.log 2>&1'`
- passed; the targeted variadic semantic-BIR route subset now includes the repaired indirect variadic call case
- proof log preserved at `test_after.log`
