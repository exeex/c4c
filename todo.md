# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 3, runtime and intrinsic families through semantic BIR

# Current Packet

## Just Finished
- widened shared scalar-type recognition, BIR type rendering, and runtime intrinsic lowering to admit long-double `x86_fp80` / `f128` signatures, then taught `lower_runtime_intrinsic_inst` to keep direct-intrinsic `@llvm.fabs.x86_fp80` wrappers on semantic BIR through the same `parse_lir_call_callee(...)` route already used for `float` / `double`
- added isolated backend route case `backend_codegen_route_x86_64_builtin_fabsl_observe_semantic_bir`, proving a simple `__builtin_fabsl` wrapper now stays on semantic BIR as `bir.call f128 llvm.fabs.x86_fp80(f128 %p.x)` without falling back to LLVM `define` output

## Suggested Next
- move to the next uncovered step-3 runtime boundary outside the `fabs` family, with the same wrapper-only discipline where needed so semantic call routing stays isolated from unrelated floating-point arithmetic or legality work

## Watchouts
- direct LLVM intrinsics are not `DirectGlobal` callees in the LIR helpers, so future semantic runtime matching must use `parse_lir_call_callee(...)` when the callee may be `llvm.*`
- wrapper-only `fabs` tests remain necessary here because `main`-body floating-point expressions still pull in unrelated arithmetic, cast, or constant-lowering work that this packet does not claim to fix
- long-double support in semantic BIR currently normalizes both `x86_fp80` and `f128` signature text to BIR `f128`, so future runtime-family matches should preserve that distinction at the callee symbol while reusing the shared BIR scalar type
- keep follow-on runtime-family widening semantic and call-shaped, not testcase-shaped: this repair was about intrinsic callee classification plus long-double signature admission, not about broad floating-point legalization

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed with `test_after.log`; backend subset result is `passed=54 failed=0 total=54`
- verified `backend_codegen_route_x86_64_builtin_fabsl_observe_semantic_bir` inside the passing run and confirmed `build/backend_route/builtin_fabsl_semantic_bir_x86_64.ll` contains `bir.call f128 llvm.fabs.x86_fp80(f128 %p.x)`
