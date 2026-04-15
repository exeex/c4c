# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 3, runtime and intrinsic families through semantic BIR

# Current Packet

## Just Finished
- widened the runtime-family boundary to VLA stack rewinds by lowering `LirStackSaveOp` and `LirStackRestoreOp` directly to semantic BIR as `llvm.stacksave` and `llvm.stackrestore`, while preserving counted VLA allocation as a semantic `llvm.dynamic_alloca.i32` placeholder in semantic-observation mode so the backedge rewind stays in BIR instead of raw LLVM fallback

## Suggested Next
- stay on plan item 3 and widen the semantic runtime-family boundary to the next source-idea intrinsic family that still escapes semantic BIR, preferably `inline asm` placeholder routing or a nearby preserved `va_list` forwarding shape

## Watchouts
- keep follow-on work inside shared semantic lowering and semantic-observation LIR preservation; do not re-expand AMD64 variadic ABI details into semantic BIR or reopen `call_decode.cpp`
- the call-site fix generalizes the existing local memcpy route to compiler-generated `LirMemcpyOp` as well as direct `memcpy` calls, so nearby packets should reuse that shared leaf-copy behavior instead of adding new variadic-case-only packing shortcuts
- the current semantic variadic route now covers `va_start`, `va_copy`, scalar `va_arg`, aggregate `va_arg`, and `va_end` as explicit BIR calls; nearby packets should preserve that semantic family grouping instead of reclassifying progress by testcase names or target ABI details
- the semantic counted-allocation placeholder is intentionally scoped to semantic-observation mode so default prepared/native backend routing does not silently change ownership of dynamic `alloca`, ABI legality, or target-specific stack management

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed with `test_after.log`; backend subset result is `passed=41 failed=0 total=41`, including `backend_codegen_route_x86_64_vla_goto_stackrestore_observe_semantic_bir`
