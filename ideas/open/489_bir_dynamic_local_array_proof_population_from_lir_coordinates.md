# BIR Dynamic Local-Array Proof Population From LIR Coordinates

Status: Open
Type: BIR semantic proof population producer idea
Parent: `ideas/closed/488_bir_dynamic_local_array_consumer_coordinate_prepared_exposure.md`
Source Evidence:
- `build/agent_state/488_step3_consumer_coordinate_exposure/summary.md`
- `build/agent_state/488_step4_residual_disposition/disposition.md`
Owning Layer: BIR semantic proof-source/path/no-clobber population using LIR producer coordinates

## Goal

Populate real proof-source, path/dominance, and index no-clobber facts for
dynamic local-array indices by binding branch/compare proof facts to dynamic
local-array path records through the committed `lir_producer_*` coordinate
surface.

## Why This Exists

Idea 488 completed the missing coordinate prerequisite: dynamic local-array
element paths now expose a truthful LIR producer-site coordinate and stable
`lir-producer:` key for address-derivation paths. Dynamic rows still remain
`missing_index_range_proof` because no producer has populated proof-source,
path/dominance, and no-clobber facts into idea 486's checker.

## In Scope

- Audit prepared branch/compare proof-source facts against dynamic local-array
  path records using `lir_producer_*` fields and `lir-producer:` keys.
- Populate normalized lower/upper bound proof-source records when branch/compare
  facts explicitly prove the dynamic index range.
- Prove path/dominance coverage from proof source to the LIR producer site.
- Prove no-clobber or same-value interval facts for the dynamic index from
  proof source to consumer.
- Feed explicit facts into the existing idea 486 range-proof checker.
- Preserve fail-closed statuses for missing proof source, unsupported predicate,
  operand-role mismatch, missing path/dominance, missing no-clobber, clobbered
  index, raw-shape-only, and target-only/final-home-only cases.

## Out Of Scope

- Redefining idea 486 checker/status vocabulary.
- Extending idea 488 coordinate exposure beyond LIR producer-site coordinates.
- Prepared traversal/BIR instruction coordinate conversion.
- Idea 484 packaging.
- Scalar local-load consumption.
- RV64/MIR lowering.
- Broad generic range analysis beyond dynamic local-array proof population.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline/log
  churn.

## Acceptance Criteria

- Focused audit identifies whether the `lir_producer_*` surface is sufficient
  to bind proof-source/path/no-clobber facts for a bounded dynamic local-array
  representative.
- The contract names accepted proof-source, path/dominance, and no-clobber
  evidence keyed by LIR producer coordinates.
- Positive coverage proves at least one real dynamic local-array index feeds
  available proof facts into the idea 486 checker.
- Negative coverage rejects missing proof source, unsupported predicate,
  operand-role mismatch, non-covering path, non-dominating proof, clobbered
  index, raw-shape-only inference, and target-only/final-home-only inference.
- Fresh residual disposition states whether idea 484 packaging can resume for
  dynamic rows or whether another lower-level proof/source owner remains first.

## Reviewer Reject Signals

Reject patches that:

- infer proof population from loop shape, branch proximity, value names,
  testcase names, dump order, final homes, or RV64 target behavior;
- treat the LIR producer instruction index as a prepared traversal/BIR
  instruction index without an explicit conversion owner;
- mark dynamic rows available without proof-source, path/dominance, and
  no-clobber facts accepted by the idea 486 checker;
- combine proof population with idea 484 packaging, scalar local-load
  consumption, RV64/MIR lowering, prepared traversal conversion, or broad
  generic range analysis;
- weaken unsupported diagnostics, expectations, allowlists, pass/fail
  accounting, runtime comparison, or baseline logs as proof of progress.
