# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 3, runtime and intrinsic families through semantic BIR

# Current Packet

## Just Finished
- added `backend_codegen_route_x86_64_inline_asm_input_local_ptr_observe_semantic_bir` for `int x = 7; int *p = &x; asm volatile("" : : "r"(p)); return *p;`, proving the stronger inline-asm route now stays in semantic BIR after the local-pointer memory repair
- the new proof establishes the expected semantic shape end to end: `bir.store_local %lv.p, ptr %lv.x`, `bir.call void llvm.inline_asm(ptr %t0)`, and a semantic dereference that still returns through the local slot route instead of raw LLVM loads

## Suggested Next
- move to the next runtime or intrinsic family in step 3 that still lacks backend semantic-BIR route coverage, keeping the same shared-lowering focus instead of reopening the now-proven local-pointer inline-asm path

## Watchouts
- keep follow-on work inside shared semantic lowering; do not route inline asm back through target-specific emitters, ABI shortcuts, or `call_decode.cpp`
- this packet proves local scalar-slot addresses survive both inline-asm argument passing and later dereference; if a follow-on shape needs local aggregate-subobject addresses or richer pointer arithmetic, extend the shared memory metadata carefully instead of adding testcase-shaped exceptions
- nearby packets should preserve the existing runtime-family grouping for `memcpy`, `memset`, `va_*`, `stacksave`, `stackrestore`, and `abs` rather than mixing inline-asm follow-up with target legality or testcase-specific backend text matching

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed with `test_after.log`; backend subset result is `passed=47 failed=0 total=47`
- verified `backend_codegen_route_x86_64_inline_asm_input_local_ptr_observe_semantic_bir` inside the passing run, proving the local-pointer inline-asm argument and the later dereference both stay on the semantic BIR local-slot route
