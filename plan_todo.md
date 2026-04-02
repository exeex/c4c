Status: Active
Source Idea: ideas/open/03_lir_to_backend_ir_refactor.md
Source Plan: plan.md

# Active Queue: LIR To Backend IR Refactor

## Queue
- [x] Step 1: Re-establish the lowering boundary
  - Notes: Moved the production lowering declarations/implementation to `src/backend/lowering/lir_to_backend_ir.*`, kept `src/backend/lir_adapter.hpp` as the legacy shim, and validated `backend_lir_adapter_tests`, `backend_lir_adapter_aarch64_tests`, `backend_lir_adapter_x86_64_tests`, `backend_ir_tests`, plus a monotonic full-suite regression check.
  - Blockers: None.
- [ ] Step 2: Isolate LIR syntax decoding
  - Notes: The lowering entrypoint/file ownership now exists. This slice removed the remaining x86-local direct-global LIR call decoders in `src/backend/x86/codegen/emit.cpp` and routed those legacy-LIR call sites through the shared `src/backend/lowering/call_decode.*` helpers; `backend_lir_adapter_tests` now covers a mismatched-callee rejection on the shared LIR-call helper path. Validation: `backend_lir_adapter_tests`, `backend_lir_adapter_x86_64_tests`, and a monotonic full-suite regression check (`2667 passed / 2 failed` before and after, with the same existing AArch64 object-contract failures).
  - Blockers: None.
- [ ] Step 3: Make backend IR more backend-native
  - Notes: Defer until decode ownership is isolated enough to expose a clean IR slice.
  - Blockers: Depends on Step 2.
- [ ] Step 4: Shift target codegen onto backend semantics
  - Notes: Defer until at least one structured operand/call slice exists in backend IR.
  - Blockers: Depends on Step 3.
- [ ] Step 5: Validate positioning for later BIR work
  - Notes: Closeout only after the active structural slice lands and backend validation is stable.
  - Blockers: Depends on prior steps.

Current Step: Step 2
Next Step: Identify the next target-side LLVM-text decode helper outside `src/backend/lowering/` after the x86 direct-call helpers and migrate that narrow slice behind the shared lowering layer
Blockers: None
