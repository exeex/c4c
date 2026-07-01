# Dynamic Local-Array Selected Proof-Edge Path Record Collector/Population

Status: Open
Type: BIR/prepared selected proof-edge path record collector/population idea
Parent: `ideas/closed/492_dynamic_local_array_selected_proof_edge_path_record_status_api.md`
Source Evidence:
- `build/agent_state/492_step3_selected_proof_edge_record_api/summary.md`
- `build/agent_state/492_step4_residual_disposition/disposition.md`
Owning Layer: Collector/population for selected proof-edge path records keyed to LIR producer coordinates

## Goal

Populate real `bir::Function::local_array_selected_proof_edge_paths` records
from prepared branch/compare facts, selected edge tuples, path coverage,
dominance/guard facts, and matching dynamic local-array
`lir_producer_lookup_key` rows.

## Why This Exists

Idea 492 added the independent record/status API surface but intentionally did
not populate real records from prepared helper inputs. Downstream selected path
certification and proof population still need explicit available/unavailable
records rather than helper-local reachability or dominance calculations.

## In Scope

- Audit existing prepared branch/compare facts, successor labels,
  reachability/path helpers, dominance/guard helpers, and local-array
  `lir_producer_lookup_key` records for a bounded collector.
- Match selected proof sources to local-array element path records by exact
  function and `lir_producer_lookup_key` identity.
- Populate available selected proof-edge path records when all required facts
  are explicit and coherent.
- Populate fail-closed unavailable records for missing/mismatched path, key,
  coordinate, role, proof source, selected edge/outcome, non-covering path,
  non-guarding proof, same-block ordering gap, unsupported boundary,
  coordinate confusion, raw-shape-only evidence, and target/final-home-only
  evidence.
- Add focused coverage that proves real collector output and fail-closed
  records without changing downstream consumers.
- Add printer/display exposure only if needed for durable probe evidence; keep
  it display-only.

## Out Of Scope

- Dynamic-index same-value/no-clobber interval effect classification.
- Populating idea 489 proof facts or idea 486 checker inputs.
- Idea 484 packaging.
- Scalar local-load consumption.
- RV64/MIR lowering.
- LIR-to-prepared/BIR same-block instruction ordering conversion beyond
  explicit unavailable status reporting.
- Broad generic range analysis.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, runtime-comparison changes, or baseline/log churn.

## Acceptance Criteria

- Step 1 identifies a bounded collector path or records the exact lower
  population blocker.
- The population contract names source facts, matching keys, selected outcome,
  edge tuple, path coverage, dominance/guard validity, and fail-closed
  statuses.
- If implementation is selected, focused tests/probes show real
  `local_array_selected_proof_edge_paths` records produced from prepared
  branch/control-flow inputs.
- Negative coverage rejects missing or mismatched keys, missing proof sources,
  unavailable LIR producer coordinates, unsupported producer role,
  non-covering paths, non-guarding proofs, same-block ordering gaps,
  coordinate confusion, raw-shape-only evidence, and target/final-home-only
  evidence.
- Residual disposition states whether selected path certification can resume or
  whether another lower collector/source owner remains first.

## Reviewer Reject Signals

Reject patches that:

- infer selected proof-edge records from testcase names, block labels, value
  names, dump order, final homes, or RV64 target behavior;
- consume helper-local reachability/dominance facts in downstream proof
  population without publishing explicit records/statuses;
- match records by fuzzy branch shape instead of exact function and
  `lir_producer_lookup_key` identity;
- treat `lir_producer_instruction_index` as a prepared traversal or BIR
  instruction index;
- combine collector/population with interval effect/no-clobber classification,
  idea 489 proof population, idea 486 checker input population, idea 484
  packaging, scalar local-load consumption, or RV64/MIR lowering;
- weaken unsupported diagnostics, expectations, allowlists, pass/fail
  accounting, runtime comparison, or baseline logs as proof of progress.
