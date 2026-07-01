# BIR Semantic Local-Address Provenance And Array-Element Access Authority

Status: Open
Type: BIR semantic producer prerequisite idea
Parent: `ideas/closed/483_bir_semantic_local_memory_scalar_load_producer.md`
Source Evidence:
- `build/agent_state/483_step1_corrected_local_load_search/audit.md`
- `build/agent_state/483_step1_corrected_local_load_search/local_load_rows.tsv`
- `review/483_step1_local_load_route_review.md`
Owning Layer: BIR semantic local-address/provenance producer

## Goal

Publish explicit BIR semantic authority for local object addresses,
array-decay/index/offset facts, element layout, and pointer-to-local-element
provenance so later scalar local-load producers can consume those facts without
guessing.

## Why This Exists

Idea 483 could not find a non-pointer/provenance ordinary scalar local-load
representative in the 79-row local-memory load bucket. The promising rows all
depend on a missing lower-level producer for local-address and array-element
access authority.

Representative blocked shapes:

- `src/pr22098-1.c`: `p#L1 = &<clit>#L3[(++a#L0)]`, then `(*p#L1) != 1`.
- `src/pr38048-1.c`: `det += a#L1[i#L3][0]` where local pointer `a = mat`.
- `src/multi-ix.c`: direct local-array element samples exist, but the row is
  contaminated by variadic/va_arg ownership and needs precise function/site
  separation before consumption.

## In Scope

- Audit focused local-address and array-element access representatives.
- Define a producer contract for local object identity, array decay, index,
  element offset/range, element type/layout/alignment, and pointer-to-local
  provenance.
- Preserve explicit rejection for integer-pointer round trips, unknown
  provenance, global sources, aggregate/member ownership, variadic/va_arg
  contamination, volatile/atomic, complex, vector, F128, runtime/call, and
  bootstrap-adjacent rows.
- Provide a durable handoff predicate or record that later local-load producers
  can consume.

## Out Of Scope

- Implementing scalar local-memory load consumption directly.
- Local-memory store, GEP/address generalization beyond the selected local
  array-element authority packet.
- Direct-call metadata, memcpy/memset byte/object-representation, alloca,
  function-signature, call-return, scalar-binop, or bootstrap repairs.
- RV64/MIR recovery from raw BIR shape, value names, testcase names, final
  homes, or target fallback inference.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline/log
  churn.

## Acceptance Criteria

- Focused probes identify whether a bounded local-address/array-element
  authority packet exists.
- The contract names all required identity, layout, range, and provenance facts.
- Positive coverage proves at least one local-address/array-element provenance
  authority record or predicate when explicit source facts exist.
- Negative coverage rejects integer-pointer round trips, unknown provenance,
  global/aggregate/member/variadic/runtime/F128/bootstrap contamination, and
  raw-shape inference.
- Fresh residual disposition states whether idea 483-style scalar local-load
  production can resume or whether another prerequisite remains first.

## Reviewer Reject Signals

Reject patches that:

- infer local-address provenance from testcase names, value names, raw dump
  order, final homes, or RV64 target fallback behavior;
- treat integer-pointer round trips such as `(*((int*)b#L2))` as valid local
  provenance;
- combine scalar load consumption, stores, generic GEP, calls, byte runtime,
  alloca, bootstrap, aggregate/member, F128, or RV64 lowering into this
  prerequisite packet;
- weaken unsupported diagnostics, expectations, allowlists, pass/fail
  accounting, runtime comparison, or baseline logs as proof of progress;
- claim classification-only route notes as implementation of durable
  provenance authority.
