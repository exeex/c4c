# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 3, runtime and intrinsic families through semantic BIR

# Current Packet

## Just Finished
- widened the runtime-family boundary to inline asm by lowering `LirInlineAsmOp` into an explicit semantic `llvm.inline_asm` BIR placeholder with preserved template, constraint, and side-effect metadata, so semantic observation no longer falls back to raw LLVM IR for a simple `asm volatile("nop")` route

## Suggested Next
- stay on plan item 3 and either broaden the inline-asm placeholder to preserve typed operand lists beyond the current raw-args fallback path, or move to the next nearby runtime family that still escapes semantic BIR such as a preserved `va_list` forwarding shape

## Watchouts
- keep follow-on work inside shared semantic lowering; do not route inline asm back through target-specific emitters, ABI shortcuts, or `call_decode.cpp`
- the new `llvm.inline_asm` BIR placeholder is intentionally semantic-only metadata today: it preserves template, constraints, side-effect state, and parsed scalar arguments when possible, while falling back to raw argument text when operand typing is not yet modeled
- nearby packets should preserve the existing runtime-family grouping for `memcpy`, `memset`, `va_*`, `stacksave`, `stackrestore`, and `abs` rather than mixing inline-asm follow-up with target legality or testcase-specific backend text matching

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed with `test_after.log`; backend subset result is `passed=42 failed=0 total=42`, including `backend_codegen_route_x86_64_inline_asm_nop_observe_semantic_bir`
