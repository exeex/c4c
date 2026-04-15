# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 3, lower runtime and intrinsic families semantically

# Current Packet

## Just Finished
- proved pointer-valued inline asm output and read/write forms already stay on the semantic BIR route without new lowering changes
- added backend route coverage for `__asm__ volatile("" : "=r"(q) : "r"(p));` and `__asm__ volatile("" : "+r"(p));`, and the semantic BIR now observes `bir.call ptr llvm.inline_asm(ptr %t0)` with `=r,r` and `+r` constraints before storing the pointer result back to the lvalue slot

## Suggested Next
- treat inline asm as likely exhausted at the current single-output carrier boundary unless the next packet explicitly expands frontend/HIR representation for tied multi-output forms
- if step 3 continues without a carrier expansion packet, move to the semantic unsupported-boundary cleanup in ordered step 4 instead of stretching inline asm with testcase-shaped coverage additions

## Watchouts
- this packet was a proof-only checkpoint: pointer outputs already lower honestly through the existing `StmtEmitter` and `lir_to_bir` path, so there was no backend/runtime code repair to land
- the remaining inline-asm gap is still representational rather than backend-only: tied digit or multi-output cases need a carrier that can name more than one write result and should not be faked by dropping outputs
- the empty-template parser fold remains intentionally separate from volatile asm preservation, so future inline-asm work still needs to keep the “all outputs must be representable” guard intact

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed with the new pointer inline-asm route cases; `backend_codegen_route_x86_64_inline_asm_output_ptr_observe_semantic_bir` and `backend_codegen_route_x86_64_inline_asm_output_readwrite_ptr_observe_semantic_bir` both observe semantic `bir.call ptr llvm.inline_asm(ptr %t0)` output paths
- proof log preserved at `test_after.log`
