# BIR Local Array Address Derivation And Index-Range Authority Carrier

Status: Open
Type: BIR semantic authority carrier/producer prerequisite idea
Parent: `ideas/closed/484_bir_semantic_local_address_provenance_array_element_authority.md`
Source Evidence:
- `build/agent_state/484_step3_local_address_authority_producer/blocker.md`
- `build/agent_state/484_step4_residual_disposition/disposition.md`
Owning Layer: BIR/HIR semantic local-array address derivation and index-range authority

## Goal

Publish durable local-array address derivation and index-range authority
carriers so later local-address/provenance and scalar local-load producers can
consume explicit facts instead of route-local lowering side tables.

## Why This Exists

Idea 484 defined the needed local-address/array-element provenance contract,
but current BIR/HIR lowering data lacks stable source object identity,
derivation identity, ordered element paths, dynamic index range proof, and
pointer-to-local-element provenance status. Implementing 484 directly would
require raw-shape inference.

The first blocked accepted shape is `src/pr38048-1.c`:

```text
decl mat: int[2][1]
decl a: int*[1] = mat#L0
det#L2 += a#L1[i#L3][0]
```

## In Scope

- Audit existing BIR/HIR local-memory lowering carriers for local arrays,
  compound literals, array decay, pointer/view derivation, dynamic indices, and
  element access paths.
- Define a durable carrier for source local object identity, object type/rank,
  layout/range, and lifetime/scope where available.
- Define explicit local-address or array-decay derivation identity and site.
- Define ordered element paths with constant and dynamic index identities.
- Define dynamic index range proof fields, including proof source and
  path/dominance validity.
- Define element layout/range and scalar in-bounds status.
- Define pointer-to-local-element provenance status and unavailable statuses.

## Out Of Scope

- Scalar local-load consumption.
- Packaging the carrier into idea 484's higher-level local-address provenance
  record.
- Generic GEP/address lowering beyond the local array-element authority carrier.
- Local-memory stores, calls, memcpy/memset, alloca, bootstrap, F128, aggregate
  member, union/object-representation, volatile/atomic, complex, vector, or
  RV64/MIR lowering.
- Accepting integer-pointer round trips as provenance-preserving.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline/log
  churn.

## Acceptance Criteria

- Focused audit identifies which existing surfaces can provide source object,
  derivation, element path, index/range, layout/range, and provenance facts.
- The carrier contract names required fields and unavailable statuses.
- Positive coverage proves a durable carrier or predicate for at least one
  local array address derivation with explicit in-bounds index/range authority.
- Negative coverage rejects missing source object, missing derivation, missing
  index/range, out-of-bounds, aggregate/member, integer-pointer round-trip,
  global, variadic/runtime, unsupported type, bootstrap, raw-shape-only, and
  target-only cases.
- Fresh residual disposition states whether idea 484 can resume packaging work
  or whether another lower-level producer remains first.

## Reviewer Reject Signals

Reject patches that:

- infer authority from route-local `element_slots`, `base_index`, lowered index
  values, type text, testcase names, value names, dump order, final homes, or
  target fallback behavior without durable producer identity;
- accept integer-pointer round trips as local element provenance;
- combine scalar local-load consumption, idea 484 packaging, stores, generic
  GEP, calls, byte runtime, alloca, bootstrap, aggregate/member, F128, or RV64
  lowering into this carrier packet;
- weaken unsupported diagnostics, expectations, allowlists, pass/fail
  accounting, runtime comparison, or baseline logs as proof of progress;
- claim classification-only route notes as implementation of the durable
  authority carrier.
