# BIR Dynamic Local-Array Proof-Source Path/No-Clobber Population

Status: Open
Type: BIR semantic proof population producer idea
Parent: `ideas/closed/486_bir_index_range_proof_path_dominance_carrier.md`
Source Evidence:
- `build/agent_state/486_step3_range_proof_path_dominance_carrier/summary.md`
- `build/agent_state/486_step4_residual_disposition/disposition.md`
Owning Layer: BIR semantic dynamic index proof-source, path, and no-clobber population

## Goal

Populate real proof-source, path/dominance, and index no-clobber facts for
dynamic local-array indices so idea 486's checker/status carrier can mark
supported dynamic local-array element paths available.

## Why This Exists

Idea 486 completed the independent checker/status surface, but real dynamic
local-array rows remain `missing_index_range_proof` because no producer yet
publishes proof-source records from control-flow branch/compare facts, proves
that the proof path covers the dynamic GEP/access consumer, or proves the index
value is unchanged along that path.

## In Scope

- Audit real control-flow branch/compare facts that could prove dynamic local
  array index bounds.
- Populate proof-source records with predicate, operands, lower/upper bounds,
  and index operand role.
- Prove path/dominance coverage from proof source to dynamic GEP/access
  consumer.
- Prove index no-clobber or same-value from proof source to consumer.
- Feed the populated facts into idea 486's checker without raw-shape inference.
- Preserve unavailable statuses for missing proof, non-dominating proof,
  uncovered path, clobbered index, unsupported comparison, raw-shape-only, and
  target-only cases.

## Out Of Scope

- Redefining idea 486's checker/status vocabulary.
- Local-array source-object, derivation, or layout carrier records completed by
  idea 485.
- Idea 484 packaging and scalar local-load consumption.
- RV64/MIR lowering.
- Broad generic range analysis beyond dynamic local-array proof population.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline/log
  churn.

## Acceptance Criteria

- Focused audit identifies a bounded real proof-source/path/no-clobber
  population packet or records the exact missing lower-level owner.
- Positive coverage proves at least one real dynamic local-array index has
  populated proof source, path/dominance coverage, and no-clobber facts that the
  checker accepts.
- Negative coverage rejects missing proof source, unsupported predicates,
  non-dominating proof, uncovered path, clobbered index, raw-shape-only, and
  target-only/final-home-only inference.
- Fresh residual disposition states whether idea 485/484 packaging can resume
  for dynamic rows or whether another lower-level producer remains first.

## Reviewer Reject Signals

Reject patches that:

- infer range proof from loop shape, variable names, testcase names, dump
  order, final homes, or RV64 target behavior;
- mark dynamic rows available without real proof-source, path/dominance, and
  no-clobber facts accepted by the idea 486 checker;
- combine proof population with idea 484 packaging, scalar local-load
  consumption, RV64 lowering, or broad generic range analysis;
- weaken unsupported diagnostics, expectations, allowlists, pass/fail
  accounting, runtime comparison, or baseline logs as proof of progress;
- claim synthetic checker coverage as real proof-source population.
