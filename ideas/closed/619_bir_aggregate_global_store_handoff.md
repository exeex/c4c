# BIR Aggregate Global Store Handoff

Status: Closed
Type: Implementation
Parent: `ideas/open/601_rv64_gcc_torture_1000_pass_recovery_umbrella.md`
Related:
- `ideas/open/603_bir_local_memory_store_semantics.md`
- `ideas/open/606_bir_global_initializer_bootstrap.md`
- `ideas/open/608_prepared_global_data_authority.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md`
Owning Layer: BIR semantic producer / prepared global-data handoff boundary
Queue Order: after local-memory producer routes that supply required source facts
Prerequisites: local aggregate subobject stores must already lower for local-only probes; do not require RV64 consumer work
Estimated Evidence Breadth: unmeasured subset of rows formerly visible through the broad store local-memory diagnostic
Proof Surface: RV64 gcc_torture backend-object rows whose first reproduced stop is aggregate value copy/store to global or static storage

## Goal

Repair or classify the BIR-to-prepared handoff for aggregate values stored into
global or static storage without folding that work into ordinary local-memory
store production.

## Why This Exists

During idea 603 Step 7, focused probes showed that local aggregate subobject
stores already emit `bir.store_local` for local-only shapes. The remaining
reproduced failures appear when those local aggregate values are copied or
stored into global/static storage, such as `u = v`, `x = <clit>`, `s[1] = t`,
or `x = foo(&i)`. That boundary is excluded from idea 603's local-memory store
route and needs its own owner contract.

## In Scope

- BIR semantic producer or prepared handoff facts for aggregate value stores
  into global/static storage.
- Source-value authority for aggregate copies whose source may be local memory,
  compound literals, call results, or array elements.
- Destination validation for supported global/static storage writes before
  RV64 consumption.
- Proof across multiple aggregate/global handoff representatives, including
  rows such as `src/pr22141-1.c`, `src/compndlit-1.c`, `src/pr57344-1.c`, and
  the `src/pr39120.c` `main` handoff if refreshed evidence still matches.

## Out Of Scope

- Ordinary local-frame scalar or aggregate subobject store production covered
  by idea 603.
- Local-memory GEP, alloca, ABI, runtime, expectation, unsupported-marker,
  allowlist, timeout, or accounting changes.
- RV64 global symbol emission or access-width lowering unless prepared
  global-data authority is already present and this idea explicitly hands off
  to a consumer route.
- Global initializer bootstrap for compile-time initializers already covered by
  idea 606.

## Acceptance Criteria

- More than one aggregate/global handoff row progresses beyond the current BIR
  semantic producer or prepared handoff stop, or reaches a defensible downstream
  owner.
- Ordinary local-only aggregate subobject store probes remain supported.
- Global-data, prepared authority, and RV64 consumer failures are not
  reclassified as local-memory stores to make the proof pass.
- The proof includes guard rows from idea 603 so local-memory store repairs do
  not regress.

## Split-In From Idea 611

Idea `611` close-readiness classified `src/ieee/20001122-1.c` and
`src/991030-1.c` as direct-object terminator residuals whose asm route stops
earlier at prepared global storage/global data layout. Treat these as candidate
global handoff rows only if refreshed diagnostics still show the global storage
or global data owner before RV64 terminator consumption.

## Reviewer Reject Signals

- Reject testcase-shaped fixes for only `src/pr22141-1.c`, `src/compndlit-1.c`,
  `src/pr57344-1.c`, `src/pr39120.c`, or any single named row.
- Reject expectation rewrites, unsupported-marker downgrades, allowlist changes,
  or diagnostic weakening claimed as aggregate/global handoff progress.
- Reject implementing aggregate/global writes inside the idea 603 local-memory
  store route without an explicit lifecycle switch.
- Reject RV64-side reconstruction of aggregate/global storage when required BIR
  or prepared authority facts are missing.
- Reject helper-only refactors that leave the same aggregate-to-global handoff
  stop under a renamed diagnostic.

## Closure Notes

Closed 2026-07-09 after the aggregate/global handoff repair in `2e150b048`
and residual classification in `9ee3252ba`.

The repaired path moves the original aggregate/global producer and prepared
handoff rows beyond the prior stop. Step 4 evidence under
`build/agent_state/619_step4_residual_classification/` shows semantic and
prepared BIR success for `src/pr22141-1.c`, `src/compndlit-1.c`,
`src/pr57344-1.c`, `src/pr39120.c`, `src/ieee/20001122-1.c`, and
`src/991030-1.c`.

Remaining failures are downstream owners, not idea 619 handoff misses:
`src/pr22141-1.c` and `src/pr39120.c` are RV64 consumer/ABI-runtime runtime
mismatches after prepared facts are present, and `src/ieee/20001122-1.c` is
prepared global-data/RV64 global memory access-width support.
