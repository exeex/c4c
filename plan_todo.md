Status: Active
Source Idea: ideas/open/03_lir_to_backend_ir_refactor.md
Source Plan: plan.md

# Active Queue: LIR To Backend IR Refactor

## Queue
- [x] Step 1: Re-establish the lowering boundary
  - Notes: Moved the production lowering declarations/implementation to `src/backend/lowering/lir_to_backend_ir.*`, kept `src/backend/lir_adapter.hpp` as the legacy shim, and validated `backend_lir_adapter_tests`, `backend_lir_adapter_aarch64_tests`, `backend_lir_adapter_x86_64_tests`, `backend_ir_tests`, plus a monotonic full-suite regression check.
  - Blockers: None.
- [ ] Step 2: Isolate LIR syntax decoding
  - Notes: The lowering entrypoint/file ownership now exists. Earlier in Step 2, the remaining x86-local direct-global LIR call decoders in `src/backend/x86/codegen/emit.cpp` were routed through the shared `src/backend/lowering/call_decode.*` helpers, and `backend_lir_adapter_tests` gained a mismatched-callee rejection on that shared LIR-call helper path. A later slice moved the x86 declared-extern typed-argument decoder out of `src/backend/x86/codegen/emit.cpp` into `src/backend/lowering/call_decode.*`, added an inferred-parameter x86 asm-path regression in `backend_lir_adapter_x86_64_tests`, and kept the direct extern-call path behavior unchanged. A later AArch64 slice moved the remaining general-emitter `args_str` and function-signature parsing behind `src/backend/lowering/call_decode.*`, removed the target-local parser from `src/backend/aarch64/codegen/emit.cpp`, and added a spacing-tolerant param-slot direct-call asm-path regression in `backend_lir_adapter_aarch64_tests`. A later slice moved the AArch64 minimal-lowering gate's raw LIR call/signature nonminimal-type detection behind shared helpers in `src/backend/lowering/call_decode.*`, replaced emitter-local string scans with those helpers, and added lowering-helper coverage in `backend_lir_adapter_tests` for i32-vs-i64 call/signature classification plus malformed-suffix fallback behavior. The previous slice moved the remaining AArch64 minimal-lowering gate type/global/return nonminimal-text predicate into shared `src/backend/lowering/call_decode.*` helpers, replaced emitter-local scans in `src/backend/aarch64/codegen/emit.cpp`, and added shared lowering coverage in `backend_lir_adapter_tests` for minimal i32 globals/returns plus floating-init and i64-return rejection. The latest slice moved repeated LIR `main` signature decoding behind new shared `src/backend/lowering/call_decode.*` helpers, replaced raw `signature_text == "define i32 @main()\\n"` and `find("i32 @main(")` checks across `src/backend/x86/codegen/emit.cpp` and `src/backend/aarch64/codegen/emit.cpp`, and added lowering-helper coverage in `backend_lir_adapter_tests` for spaced zero-arg `main`, parameterized `main`, declaration rejection, and non-`i32` rejection. Validation: `./build/backend_lir_adapter_tests`, `./build/backend_lir_adapter_x86_64_tests`, `./build/backend_lir_adapter_aarch64_tests`, and `./build/backend_ir_tests` passed; the monotonic full-suite regression guard also passed with `2667 passed / 2 failed` before and after, with the same existing AArch64 object-contract failures and no newly failing tests.
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
Next Step: Re-audit the remaining x86 helper-signature string matches and the AArch64 legacy fallback reparses of `fn.signature_text`/`p->args_str`; if those are the last decode seams, either finish Step 2 with shared helpers or promote the generic fallback cleanup into the first Step 3 structured backend-IR slice
Blockers: None
