# BIR Index Range Proof And Path-Dominance Carrier

Status: Open
Type: BIR semantic proof carrier/producer prerequisite idea
Parent: `ideas/closed/485_bir_local_array_address_derivation_index_range_authority_carrier.md`
Source Evidence:
- `build/agent_state/485_step3_local_array_carrier_producer/summary.md`
- `build/agent_state/485_step4_residual_disposition/disposition.md`
Owning Layer: BIR semantic index range proof and path/dominance authority

## Goal

Publish durable dynamic index range proof, path/dominance validity, and
index-value no-clobber carrier facts so local array element-path carriers can
mark dynamic local-array GEP rows available without raw-shape inference.

## Why This Exists

Idea 485 added source-object, derivation, and layout carrier records, but
dynamic local-array GEP rows remain unavailable as `missing_index_range_proof`.
Rows such as `pr38048-1.c` require proof that the dynamic index is in range at
the consumer access and that the proof dominates and covers the consumer path.

## In Scope

- Audit existing BIR/HIR/control-flow data for dynamic index range proof
  candidates.
- Define records for proof source identity, index value identity, lower/upper
  bounds, comparison predicate, and operand identity.
- Define path/dominance validity from proof source to consumer access.
- Define index no-clobber or same-value validity between proof and consumer.
- Publish unavailable statuses for missing proof, non-dominating proof,
  uncovered path, clobbered index, unsupported comparison, raw-shape-only, and
  target-only cases.
- Preserve a fail-closed result when a loop condition exists textually but is
  not tied to the consumer path.

## Out Of Scope

- Local-array source-object, derivation, or layout carrier records already
  completed by idea 485.
- Idea 484 packaging and scalar local-load consumption.
- RV64/MIR lowering.
- Generic range analysis beyond dynamic local-array index proof needed by the
  local array carrier path.
- Integer-pointer round-trip, global, aggregate/member, variadic/runtime, F128,
  complex/vector, volatile/atomic, bootstrap, raw-shape-only, and target-only
  repair.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline/log
  churn.

## Acceptance Criteria

- Focused audit identifies whether current control-flow/proof surfaces can
  provide range proof and path/dominance facts for a dynamic local-array index.
- The carrier contract names required fields and unavailable statuses.
- Positive coverage proves at least one dynamic index has explicit in-range
  proof at the consumer access with path/dominance and no-clobber validity.
- Negative coverage rejects missing proof, non-dominating proof, uncovered
  paths, clobbered indices, unsupported comparisons, raw-shape-only inference,
  and target-only/final-home-only inference.
- Fresh residual disposition states whether idea 485/484 can resume packaging
  dynamic local-array authority or whether another lower-level producer remains
  first.

## Reviewer Reject Signals

Reject patches that:

- infer index range from loop shape, variable names, testcase names, dump
  order, final homes, or RV64 target behavior;
- mark dynamic GEP rows available without proof source, path/dominance, and
  no-clobber carrier facts;
- combine this proof carrier with scalar local-load consumption, idea 484
  packaging, RV64 lowering, or broad generic range analysis;
- weaken unsupported diagnostics, expectations, allowlists, pass/fail
  accounting, runtime comparison, or baseline logs as proof of progress;
- claim classification-only route notes as implementation of dynamic range
  proof authority.
