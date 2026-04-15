# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 3, runtime and intrinsic families through semantic BIR

# Current Packet

## Just Finished
- proved the next inline-asm semantic slice with `backend_codegen_route_x86_64_inline_asm_input_i32_observe_semantic_bir`, showing an operand-carrying `asm volatile("" : : "r"(x))` lowers to `bir.call void llvm.inline_asm(i32 ...)` and clears the raw-args fallback for this typed scalar-input route
- widened that same semantic route to a nearby multi-input shape with `backend_codegen_route_x86_64_inline_asm_input_two_i32_observe_semantic_bir`, proving two typed scalar register operands lower to `bir.call void llvm.inline_asm(i32 ..., i32 ...)` without falling back to raw LLVM inline asm text

## Suggested Next
- stay on plan item 3 and push the inline-asm family to a pointer-input route next; probing shows `asm volatile("" : : "r"(p))` for `int *p = &x;` still escapes semantic BIR and emits raw LLVM inline asm, so the next packet should repair typed pointer operands in shared lowering before moving on to a different runtime family

## Watchouts
- keep follow-on work inside shared semantic lowering; do not route inline asm back through target-specific emitters, ABI shortcuts, or `call_decode.cpp`
- the current proof now covers one- and two-input scalar register operands for inline asm, but pointer operands and mixed-constraint forms still need explicit semantic coverage before the inline-asm family can be considered broadly defended
- nearby packets should preserve the existing runtime-family grouping for `memcpy`, `memset`, `va_*`, `stacksave`, `stackrestore`, and `abs` rather than mixing inline-asm follow-up with target legality or testcase-specific backend text matching

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed with `test_after.log`; backend subset result is `passed=44 failed=0 total=44`, adding `backend_codegen_route_x86_64_inline_asm_input_two_i32_observe_semantic_bir` while keeping the existing inline-asm semantic tests green
