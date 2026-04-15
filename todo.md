# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 3, runtime and intrinsic families through semantic BIR

# Current Packet

## Just Finished
- finished the semantic floating-point variadic slice by widening the preserved `va_*` observation route to `double`, teaching semantic BIR to keep floating local slots and floating call immediates honest, and covering `variadic_double_bytes` without falling back to raw LLVM IR

## Suggested Next
- stay on plan item 3 and widen the same semantic `va_*` family beyond scalar routes, preferably the existing aggregate `va_arg` route in `variadic_pair_second` or a semantic `va_copy` slice that can reuse the preserved observation path without reintroducing ABI-shaped lowering into BIR

## Watchouts
- keep follow-on work inside shared semantic lowering and semantic-observation LIR preservation; do not re-expand AMD64 variadic ABI details into semantic BIR or reopen `call_decode.cpp`
- the new preservation hook is observation-only: non-semantic backend codegen still keeps the existing AMD64 `va_arg` expansion path, so follow-on packets should preserve that separation unless the prepared-BIR route grows real variadic support
- semantic `va_arg` lowering now covers scalar integer and `double` observation cases, including floating immediates at the call site and byte-view access through address-taken floating locals; aggregate and richer variadic families remain separate work and should be widened semantically rather than by pattern-matching expanded GEP/memcpy blocks

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed with `test_after.log`; backend subset result is `passed=38 failed=0 total=38`, including `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`
