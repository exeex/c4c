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
  - Notes: Step 3 has already delivered meaningful structure around call metadata, signatures, linkage, globals, address provenance, and local-slot metadata. The local-slot branch-only constant-return family is no longer blocked on the old dead-end constraints: it now lowers without a hard local-slot-count cap, and it no longer requires the function to be named `main` or to have zero fixed parameters when those parameters are irrelevant to the matched local-slot state. This topic should not consume more plan churn. Representative coverage now replaces the old slot-count regression ladder in the shared lowering and AArch64 direct-vs-lowered parity tests, while keeping the non-`main`/unused-parameter boundary regression. The latest slice added a real typed-scalar extension for this family: shared lowering now accepts `i8` local-slot load/store values alongside `i32`, with matching shared and AArch64 regression coverage. That support is intentionally bounded; widening into `i64` would wrongly absorb the mixed-width control-flow slice that is supposed to stay on the general emitter path. Do not spend more time on size-only widening, entrypoint naming, unused-parameter gating, or broad scalar-width expansion without a concrete backend-semantic need.
  - Blockers: None.
- [x] Step 4: Shift target codegen onto backend semantics
  - Notes: The backend-side single-function fast path now accepts structured `i32` return slices even when the lowered function is not named `main` and carries unused fixed parameters, so the active local-slot family stays on the asm path after lowering-owned canonicalization instead of falling back to printed backend IR. Coverage now proves this on both targets: AArch64 gets a lowered non-`main` local-slot signature-shim regression, and x86 gets both lowered structured coverage and direct-vs-lowered parity for the same active family.
  - Blockers: None.
- [ ] Step 5: Validate positioning for later BIR work
  - Notes: Step 3/4 are structurally complete for the active family. The remaining work is closeout-oriented: document the intentionally retained transitional seams, confirm the resulting boundary is a clean precursor to later `backend_ir -> bir` work, and fold those durable conclusions back into the source idea before lifecycle close.
  - Blockers: None.

## Current Focus

Step 5: close out the refactor by documenting the retained transitional seams and BIR-facing boundary

## Immediate Next Slice

Fold the Step 3/4 conclusions back into the linked idea, explicitly note that the active local-slot family now stays on structured backend semantics across both AArch64 and x86 coverage, and bound any remaining transition shims that are intentionally left for later `backend_ir -> bir` work.

## Accomplished Context

- Shared lowering owns the main production boundary.
- Call/signature decoding moved behind lowering-owned helpers.
- Backend IR now carries more structured metadata for calls, linkage, globals, addresses, and local slots.
- AArch64 and x86 emitters already consume lowering-owned structured views for several direct-call and fast-path seams.
- The active local-slot family has been proven stable across representative widening up to eighteen slots.
- The active local-slot family now also accepts non-`main` functions, unused fixed parameters, and more than eighteen local slots without reopening the old fallback path.
- The remaining open question for this family is semantic typing, not entrypoint naming or local-slot cardinality.
- Shared lowering and AArch64 parity coverage now use representative slot-count cases instead of an exhaustive regression ladder for this family.
- Shared lowering now also normalizes the same family when the local-slot state is carried through `i8` values instead of only `i32`.
- The typed-scalar extension is intentionally bounded to backend-supported scalar locals (`i8`/`i32`) so mixed-width `i64` control-flow slices keep using the general AArch64 emitter.
- Backend-side minimal return fast paths now consume the lowered function symbol and tolerate unused fixed parameters, which keeps the non-`main` structured local-slot slice on the asm path instead of falling back to rendered backend IR.
- x86 coverage now explicitly checks both direct-vs-lowered parity and lowered structured fast-path retention for the non-`main` local-slot family, matching the AArch64 backend-semantics guarantee.

## Risks And Constraints

- Do not replace the bounded matcher with a generic direct-LIR interpreter.
- Do not broaden this plan into unrelated backend correctness work.
- Keep direct-vs-lowered emitter parity for the active family.
- Do not reintroduce arbitrary gating by symbol name, unused fixed parameters, or local-slot count for this family.
- Do not create new work items whose only effect is widening fixture cardinality without changing backend semantics.
- Preserve current workspace regression ceiling unless a separate idea is opened for unrelated failures.

## Validation Baseline

- Targeted backend tests for `backend_lir_adapter_tests`, `backend_lir_adapter_aarch64_tests`, and `backend_lir_adapter_x86_64_tests` pass after rebuild.
- The current full-suite workspace ceiling remains `2667 passed / 2 failed / 2669 total`.
- The known remaining failures are `backend_contract_aarch64_return_add_object` and `backend_contract_aarch64_global_load_object`.
- This slice revalidated `backend_lir_adapter_aarch64_tests`, `backend_lir_adapter_x86_64_tests`, and a full `ctest --test-dir build -j --output-on-failure` run. The regression guard reported no new failures and preserved the same known failing tests: `backend_contract_aarch64_return_add_object` and `backend_contract_aarch64_global_load_object`.
