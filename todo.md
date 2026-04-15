# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 3, runtime and intrinsic families through semantic BIR

# Current Packet

## Just Finished
- added `backend_codegen_route_x86_64_inline_asm_input_ptr_observe_semantic_bir`, proving a typed pointer register operand already lowers through shared semantic BIR as `bir.call void llvm.inline_asm(ptr ...)` when the pointer source itself stays on a supported route
- isolated the earlier `asm volatile("" : : "r"(p))` probe failure to a separate local-pointer or dereference route outside this packet: `int *p = &x; return *p;` still falls back to raw LLVM even without inline asm, so that shape is not evidence of an inline-asm lowering gap

## Suggested Next
- stay on plan item 3 but move the next packet to the actual blocker surfaced by this probe: repair semantic local-pointer load/store or dereference lowering in the memory path, then re-check the stronger inline-asm local-pointer shape after that support lands

## Watchouts
- keep follow-on work inside shared semantic lowering; do not route inline asm back through target-specific emitters, ABI shortcuts, or `call_decode.cpp`
- inline-asm pointer operands are now explicitly proved for a supported pointer-param route, but local pointer temporaries and dereference-heavy pointer flows still fall out through memory lowering before inline asm can be judged on those shapes
- nearby packets should preserve the existing runtime-family grouping for `memcpy`, `memset`, `va_*`, `stacksave`, `stackrestore`, and `abs` rather than mixing inline-asm follow-up with target legality or testcase-specific backend text matching

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed with `test_after.log`; backend subset result is `passed=45 failed=0 total=45`, adding `backend_codegen_route_x86_64_inline_asm_input_ptr_observe_semantic_bir` while keeping the existing inline-asm semantic tests green
