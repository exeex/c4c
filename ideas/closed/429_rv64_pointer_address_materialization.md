# RV64 Pointer Cast And Address Materialization

Status: Closed
Type: Implementation idea
Parent: `ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md`
Source Evidence: `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`
Owning Layer: RV64 pointer/address lowering with producer-gap boundaries
Handoff Directory: `docs/rv64_gcc_torture_post_contract/`

## Goal

Lower coherent prepared pointer cast and address materialization fragments in
RV64 while rejecting rows whose pointer provenance belongs to producer repair.

## Why This Exists

The regenerated RV64 gcc_torture scan classified 21
`unsupported_instruction_fragment` rows as pointer cast and address
materialization. Representative rows include `src/930930-1.c`,
`src/20000622-1.c`, `src/20010329-1.c`, `src/20011019-1.c`, and
`src/20041112-1.c`; `src/930930-1.c` exposes a prepared `bir.inttoptr` row.

## In Scope

- Lower coherent prepared `inttoptr` and `ptrtoint` fragments through RV64
  pointer/address materialization.
- Cover address-bearing scalar rows whose provenance and object facts are
  explicit enough for target lowering.
- Add focused tests for pointer-to-integer, integer-to-pointer, and address
  materialization shapes with coherent prepared facts.
- Preserve fail-closed diagnostics for missing provenance, object identity,
  relocation, or addressability facts.

## Out Of Scope

- Guessing addresses or object identities in RV64 lowering.
- Repairing BIR/prepared pointer provenance producers.
- Global memory/addressing residual rows that need separate addressability
  review.
- Aggregate ABI, call publication, select/join, F128, or scalar floating-point
  lowering.
- Expectation, allowlist, unsupported-marker, runtime-comparison, or pass/fail
  accounting changes.

## Acceptance Criteria

- Coherent prepared pointer cast rows lower without
  `unsupported_instruction_fragment`.
- Rows with incomplete provenance remain producer-owned and fail closed.
- Tests demonstrate a semantic pointer/address rule rather than a
  representative-file shortcut.
- The proof includes focused pointer/address tests and the
  supervisor-delegated regression subset.

## Reviewer Reject Signals

- Reject RV64 code that guesses object identity, provenance, relocation base,
  or addressability from integer shape alone.
- Reject classifying missing prepared provenance as target-lowering work.
- Reject filename-shaped handling for `930930-1.c`, `20000622-1.c`, or other
  representative rows.
- Reject expectation rewrites, unsupported downgrades, or pass/fail accounting
  changes as pointer/address progress.
- Reject a broad memory-addressing rewrite that silently absorbs global-data,
  stack-frame, or producer-contract defects.

## Completion Notes

Closed on 2026-06-30 as a bounded target-side pointer/address materialization
idea.

Completed coverage:

- Focused RV64 backend coverage now proves coherent scalar pointer-cast
  movement for `IntToPtr` from `i32`/`i64` to `ptr` and `PtrToInt` from `ptr`
  to `i64`.
- The implemented route is restricted to authoritative GPR-compatible prepared
  homes plus literal or rematerializable integer sources.
- Existing RV64 routes already consume explicit prepared frame-slot, string,
  and direct-global address materialization facts; Step 3 close-readiness found
  no additional representative-owned prepared address-materialization packet.

Representative close-readiness probes under
`build/agent_state/429_step4_close_readiness/` still show full-row failures,
but those failures are not remaining in-scope pointer-cast target-lowering
work:

- `930930-1`: pointer-value memory/global-store/select residual.
- `20000622-1`: covered immediate `inttoptr` shape plus unrelated
  call/select/local-publication residuals.
- `20010329-1`: covered immediate/register `inttoptr` shapes plus
  terminator/select-publication residuals.
- `20011019-1`: global object data plus global pointer load/store and
  direct-global select-chain work.
- `20041112-1`: direct `bir.ret ptr @global`, global load/store,
  direct-global select-chain, and terminator residuals.

Durable follow-up:
`ideas/open/433_rv64_global_select_pointer_memory_residuals.md`.

Close proof:

- Existing canonical `test_before.log` and `test_after.log` cover the backend
  subset with `327 passed, 0 failed` before and after.
- The regression guard passed with non-decreasing pass-count semantics:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`.
