Status: Active
Source Idea: ideas/open/03_lir_to_backend_ir_refactor.md
Source Plan: plan.md

# Active Queue: LIR To Backend IR Refactor

## Queue
- [x] Step 1: Re-establish the lowering boundary
  - Notes: Moved the production lowering declarations/implementation to `src/backend/lowering/lir_to_backend_ir.*`, kept `src/backend/lir_adapter.hpp` as the legacy shim, and validated `backend_lir_adapter_tests`, `backend_lir_adapter_aarch64_tests`, `backend_lir_adapter_x86_64_tests`, `backend_ir_tests`, plus a monotonic full-suite regression check.
  - Blockers: None.
- [ ] Step 2: Isolate LIR syntax decoding
  - Notes: The lowering entrypoint/file ownership now exists. Earlier in Step 2, the remaining x86-local direct-global LIR call decoders in `src/backend/x86/codegen/emit.cpp` were routed through the shared `src/backend/lowering/call_decode.*` helpers, and `backend_lir_adapter_tests` gained a mismatched-callee rejection on that shared LIR-call helper path. This slice moved the x86 declared-extern typed-argument decoder out of `src/backend/x86/codegen/emit.cpp` into `src/backend/lowering/call_decode.*`, added an inferred-parameter x86 asm-path regression in `backend_lir_adapter_x86_64_tests`, and kept the direct extern-call path behavior unchanged. Validation: `./build/backend_lir_adapter_x86_64_tests`, plus a monotonic full-suite regression check (`2667 passed / 2 failed` before and after, with the same existing AArch64 object-contract failures).
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
Next Step: Migrate the next AArch64 target-local typed-call decoder behind `src/backend/lowering/`, starting with the fallback-path `args_str`/signature parsing helpers in `src/backend/aarch64/codegen/emit.cpp`
Blockers: None
