# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 3, runtime and intrinsic families through semantic BIR

# Current Packet

## Just Finished
- repaired local pointer temporary tracking in shared memory lowering so storing a local address like `&x` into a pointer slot and later loading it back preserves the pointee slot route through semantic BIR instead of falling back to raw LLVM memory ops
- added `backend_codegen_route_x86_64_local_pointer_deref_observe_semantic_bir` for `int x = 7; int *p = &x; return *p;`, proving the pointer temporary still loads from `%lv.p` while the dereference resolves semantically to `bir.load_local i32 %lv.x`

## Suggested Next
- re-run the stronger inline-asm local-pointer route that originally exposed this gap, now that local pointer temporaries can survive the shared memory path without raw LLVM fallback

## Watchouts
- keep follow-on work inside shared semantic lowering; do not route inline asm back through target-specific emitters, ABI shortcuts, or `call_decode.cpp`
- this packet only tracks local scalar-slot addresses through pointer locals; if a follow-on shape needs local aggregate-subobject addresses or richer pointer arithmetic, extend the shared memory metadata carefully instead of adding testcase-shaped exceptions
- nearby packets should preserve the existing runtime-family grouping for `memcpy`, `memset`, `va_*`, `stacksave`, `stackrestore`, and `abs` rather than mixing inline-asm follow-up with target legality or testcase-specific backend text matching

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed with `test_after.log`; backend subset result is `passed=46 failed=0 total=46`
- verified both repaired shapes inside the passing run: `backend_codegen_route_x86_64_local_pointer_deref_observe_semantic_bir` stays semantic for `int *p = &x; return *p;`, and `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir` is restored with semantic local-slot byte-address loads from the same shared memory-lowering route
