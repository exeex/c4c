# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 3, runtime and intrinsic families through semantic BIR

# Current Packet

## Just Finished
- finished the simple semantic variadic int slice by preserving `va_arg` on the semantic-BIR observation path, lowering `va_start`/`va_arg`/`va_end` into honest BIR call-family operations, and covering `variadic_sum2` with a backend route test that keeps the variadic call-site argument visible

## Suggested Next
- stay on plan item 3 and widen the same semantic `va_*` family beyond the simple integer case, preferably another scalar route such as floating-point or `va_copy` that can reuse the new semantic-observation preservation path without reintroducing ABI-shaped lowering into BIR

## Watchouts
- keep follow-on work inside shared semantic lowering and semantic-observation LIR preservation; do not re-expand AMD64 variadic ABI details into semantic BIR or reopen `call_decode.cpp`
- the new preservation hook is observation-only: non-semantic backend codegen still keeps the existing AMD64 `va_arg` expansion path, so follow-on packets should preserve that separation unless the prepared-BIR route grows real variadic support
- semantic `va_arg` lowering is still intentionally narrow to scalar/ptr-style observation cases; aggregate and richer variadic families remain separate work and should be widened semantically rather than by pattern-matching expanded GEP/memcpy blocks

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed with `test_after.log`; backend subset result is `passed=37 failed=0 total=37`, including `backend_codegen_route_x86_64_variadic_sum2_observe_semantic_bir`
