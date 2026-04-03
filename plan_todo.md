Status: Active
Source Idea: ideas/open/03_lir_to_backend_ir_refactor.md
Source Plan: plan.md

# Active Queue: LIR To Backend IR Refactor

## Queue
- [x] Step 1: Re-establish the lowering boundary
  - Notes: Production ownership moved to `src/backend/lowering/lir_to_backend_ir.*`, with `src/backend/lir_adapter.hpp` kept as a compatibility shim.
  - Blockers: None.
- [x] Step 2: Isolate LIR syntax decoding
  - Notes: Shared decode/parsing helpers in `src/backend/lowering/call_decode.*` now own the staged LIR call and signature parsing previously spread across target codegen.
  - Blockers: None.
- [ ] Step 3: Make backend IR more backend-native
  - Notes: Step 3 has already delivered meaningful structure around call metadata, signatures, linkage, globals, address provenance, and local-slot metadata. The local-slot branch-only constant-return family is no longer blocked on the old dead-end constraints: it now lowers without a hard local-slot-count cap, and it no longer requires the function to be named `main` or to have zero fixed parameters when those parameters are irrelevant to the matched local-slot state. This topic should not consume more plan churn. The only worthwhile remaining work for this family is: `1.` decide whether to generalize the current `i32`-only local-slot/value model into typed-scalar support because a real backend semantic family needs it; `2.` collapse repetitive slot-count fixtures into representative semantic coverage. Do not spend more time on size-only widening, entrypoint naming, or unused-parameter gating.
  - Blockers: None.
- [ ] Step 4: Shift target codegen onto backend semantics
  - Notes: Recent AArch64 and x86 work already removed many emitter-local reshaping layers for direct-call and structured fast-path seams. Further Step 4 cleanup for this active family depends on Step 3 producing a stable structured lowering result instead of another bounded compatibility matcher.
  - Blockers: Depends on Step 3 structural convergence for the active family.
- [ ] Step 5: Validate positioning for later BIR work
  - Notes: Closeout only after the active structural family lands and backend validation stays stable.
  - Blockers: Depends on Steps 3 and 4.

## Current Focus

Step 3

## Immediate Next Slice

Do not revisit `main`/parameter/local-slot-count gating for this family. If more work is needed here, it must either `1.` generalize the `i32`-only model for a real typed-scalar/backend-semantic reason, or `2.` replace repetitive slot-count tests with representative semantic coverage.

## Accomplished Context

- Shared lowering owns the main production boundary.
- Call/signature decoding moved behind lowering-owned helpers.
- Backend IR now carries more structured metadata for calls, linkage, globals, addresses, and local slots.
- AArch64 and x86 emitters already consume lowering-owned structured views for several direct-call and fast-path seams.
- The active local-slot family has been proven stable across representative widening up to eighteen slots.
- The active local-slot family now also accepts non-`main` functions, unused fixed parameters, and more than eighteen local slots without reopening the old fallback path.
- The remaining open question for this family is semantic typing, not entrypoint naming or local-slot cardinality.

## Risks And Constraints

- Do not replace the bounded matcher with a generic direct-LIR interpreter.
- Do not broaden this plan into unrelated backend correctness work.
- Keep direct-vs-lowered emitter parity for the active family.
- Do not reintroduce arbitrary gating by symbol name, unused fixed parameters, or local-slot count for this family.
- Do not create new work items whose only effect is widening fixture cardinality without changing backend semantics.
- Preserve current workspace regression ceiling unless a separate idea is opened for unrelated failures.

## Validation Baseline

- Targeted backend tests for `backend_lir_adapter_tests` and `backend_lir_adapter_aarch64_tests` have been passing on the recent Step 3 slices after rebuild.
- The current full-suite workspace ceiling remains `2667 passed / 2 failed / 2669 total`.
- The known remaining failures are `backend_contract_aarch64_return_add_object` and `backend_contract_aarch64_global_load_object`.
