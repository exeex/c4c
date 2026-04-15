# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 3, runtime and intrinsic families through semantic BIR

# Current Packet

## Just Finished
- widened shared semantic call-signature lowering to accept `float` / `double` params and returns, then taught `lower_runtime_intrinsic_inst` to recognize direct-intrinsic `@llvm.fabs.double` / `@llvm.fabs.float` calls through `parse_lir_call_callee(...)` plus inferred typed args instead of the direct-global parser
- added isolated backend route cases `backend_codegen_route_x86_64_builtin_fabs_f64_observe_semantic_bir` and `backend_codegen_route_x86_64_builtin_fabs_f32_observe_semantic_bir`, proving simple wrapper functions now stay on semantic BIR as `bir.call double llvm.fabs.double(double %p.x)` and `bir.call float llvm.fabs.float(float %p.x)` without falling back to LLVM `define` output

## Suggested Next
- decide whether the same runtime-family packet should widen to `__builtin_fabsl` or stop here and move to the next uncovered step-3 runtime boundary, keeping any follow-on proof reduced to wrapper-only cases until broader floating-point semantic lowering is intentionally in scope

## Watchouts
- direct LLVM intrinsics are not `DirectGlobal` callees in the LIR helpers, so future semantic runtime matching must use `parse_lir_call_callee(...)` when the callee may be `llvm.*`
- wrapper-only `fabs` tests were necessary here because adding `main`-body float negation or float-to-int conversion pulls in unrelated floating-point lowering that this packet does not claim to fix
- keep follow-on runtime-family widening semantic and call-shaped, not testcase-shaped: this repair was about intrinsic callee classification and shared float signature support, not about broad float arithmetic legalization

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed with `test_after.log`; backend subset result is `passed=53 failed=0 total=53`
- verified `backend_codegen_route_x86_64_builtin_fabs_f64_observe_semantic_bir` and `backend_codegen_route_x86_64_builtin_fabs_f32_observe_semantic_bir` inside the passing run, proving simple `fabs` wrappers now stay on the semantic BIR route without LLVM fallback
