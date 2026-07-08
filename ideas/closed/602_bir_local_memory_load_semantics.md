# BIR Local-Memory Load Semantics

Status: Closed
Type: Implementation
Parent: `ideas/closed/601_rv64_gcc_torture_1000_pass_recovery_umbrella.md`
Related:
- `docs/rv64_gcc_torture_1000_pass_recovery/current_scan_summary.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md`
Owning Layer: BIR semantic producer
Queue Order: 1
Prerequisites: current `470/1467` RV64 gcc_torture evidence; no RV64 target fixes required before this producer repair
Estimated Evidence Breadth: `82` local-memory load rows, with adjacent local-memory rows expected to become better classified after repair
Proof Surface: RV64 gcc_torture backend-object rows that stop in the `load local-memory semantic family`, plus nearby local-memory rows that must retain accurate first-owner diagnostics

## Closure Note

Closed on 2026-07-08 after the active runbook completed all five steps and
satisfied the source idea's bounded acceptance criteria. The implementation
changed only `src/backend/bir/lir_to_bir/memory/local_slots.cpp`, adding a
narrow BIR producer fact for successful loads from a pointer-valued global when
no known pointee/global alias exists. No RV64 target lowering, tests,
expectations, unsupported markers, allowlists, runtime/timeout/accounting
behavior, store/GEP/alloca code, or downstream ABI/runtime policy changed.

Concrete proof rows moved past the original BIR producer stop:

- `src/20000706-4.c` cleared the old `load local-memory semantic family` stop
  and now reaches `[RV64_BACKEND_RUNTIME_MISMATCH]` with a segmentation-fault
  runtime result.
- `src/20010129-1.c` cleared the old `load local-memory semantic family` stop
  and now reaches prepared/RV64 `unsupported_call_abi`.

The restored full RV64 gcc_torture backend-object scan reports `473/1467`
passing with `994` failures. The local-memory load semantic family is now `73`
remaining stops, down from the Step 2 baseline of `82`; this proves aggregate
same-family movement, although overwritten mutable pre-repair logs do not
preserve exact identities for every unnamed moved row.

Adjacent guard rows retained non-load ownership: `src/20010605-2.c` stayed in
the store local-memory family, `src/20030717-1.c` stayed in the GEP local-memory
family, `src/20180921-1.c` stayed in the alloca local-memory family,
`src/20000217-1.c` stayed at prepared move-bundle classification,
`src/20021204-1.c` stayed at RV64 `unsupported_local_memory_access`,
`src/20030910-1.c` stayed at `unsupported_terminator_fragment`, and
`src/20000706-1.c` stayed at move-bundle/stack-offset publication rejection.

Remaining `load local-memory semantic family` rows are not closure blockers for
this idea because the acceptance criteria required multiple rows to progress,
guard ownership to remain accurate, and matching RV64 backend-object proof.
The remaining load rows are follow-up BIR producer breadth for
aggregate/member, `va_arg`/byval, or complex pointer-load shapes. The two
cleared rows expose downstream runtime mismatch and ABI/RV64 ownership, which
remain out of scope for this source idea.

## Goal

Repair BIR production of local-memory load semantics for ordinary C cases so
prepared and RV64 stages receive valid memory-use facts instead of stopping at
the semantic producer layer.

## Why This Exists

The current failure map identifies `82` local-memory load rows as the largest
single BIR producer diagnostic family. Target-local RV64 work cannot safely
consume loads that BIR never modeled.

## In Scope

- BIR load semantics for local frame memory and scalar values.
- Diagnostic-preserving handling for nearby rows whose address, store, GEP, or
  alloca authority is still missing.
- Same-family proof across multiple gcc_torture rows, not one named case.

## Out Of Scope

- Store, GEP, alloca, global initializer, RV64 consumer, ABI, runtime,
  expectation, unsupported-marker, allowlist, timeout, or accounting changes.
- Treating final assembly shape as proof of BIR memory semantics.

## Acceptance Criteria

- Multiple rows from the local-memory load diagnostic family progress past the
  original BIR producer stop.
- Rows whose first owner is store, GEP, alloca, prepared authority, or RV64
  consumption remain accurately classified instead of being forced through load
  repair.
- The proof uses the RV64 gcc_torture backend-object route or a narrower
  matching subset that exercises the same failing family.

## Reviewer Reject Signals

- Reject testcase-shaped matching against representative case names such as
  `src/20041124-1.c`.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime
  comparison, or pass/fail accounting changes as progress.
- Reject RV64 target inference that papers over missing BIR load facts.
- Reject a broad local-memory rewrite that also changes store, GEP, alloca,
  ABI, or runtime behavior without separate ownership.
- Reject helper renames or diagnostic wording changes that leave the original
  load semantic failure mode intact.
