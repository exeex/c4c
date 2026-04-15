# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 3, lower runtime and intrinsic families semantically

# Current Packet

## Just Finished
- preserved single-output read/write inline asm through semantic BIR by carrying the `+r` source value into `LirInlineAsmOp` before storing the result back to the lvalue
- added the scalar `i32` backend route proof for `__asm__ volatile("" : "+r"(x));`, and the semantic BIR now observes `bir.call i32 llvm.inline_asm(i32 %t0) [asm="", constraints="+r", sideeffect]`

## Suggested Next
- decide whether the next honest inline-asm packet is digit-tied multi-output support or whether inline-asm should pause here and step 3 should move to another runtime/intrinsic family
- if inline-asm continues, keep the next slice focused on semantics the current single-output HIR/LIR route can actually represent without inventing testcase-shaped special handling

## Watchouts
- the new `+r` path still proves only single-output scalar lvalues; multi-output asm is still outside the current HIR carrier and should not be faked by dropping write results
- the empty-template parser fold remains intentionally separate from volatile asm preservation, so future inline-asm work still needs to keep the “all outputs must be representable” guard intact

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed after the `+r` lowering fix and backend proof-case addition; the new route test now observes `bir.call i32 llvm.inline_asm(i32 %t0)` in semantic BIR for the single-output read/write path
- proof log preserved at `test_after.log`
