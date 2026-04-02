Status: Active
Source Idea: ideas/open/03_lir_to_backend_ir_refactor.md
Source Plan: plan.md

# Active Queue: LIR To Backend IR Refactor

## Queue
- [x] Step 1: Re-establish the lowering boundary
  - Notes: Moved the production lowering declarations/implementation to `src/backend/lowering/lir_to_backend_ir.*`, kept `src/backend/lir_adapter.hpp` as the legacy shim, and validated `backend_lir_adapter_tests`, `backend_lir_adapter_aarch64_tests`, `backend_lir_adapter_x86_64_tests`, `backend_ir_tests`, plus a monotonic full-suite regression check.
  - Blockers: None.
- [ ] Step 2: Isolate LIR syntax decoding
  - Notes: The lowering entrypoint/file ownership now exists; next slice should move additional LLVM-text decode helpers behind the lowering layer without expanding target-side parsing.
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
Next Step: Identify the next backend-owned LLVM-text decode helper still exposed outside `src/backend/lowering/` and migrate that narrow slice behind the lowering layer
Blockers: None
