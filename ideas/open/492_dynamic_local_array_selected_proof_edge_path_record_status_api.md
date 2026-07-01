# Dynamic Local-Array Selected Proof-Edge Path Record/Status API

Status: Open
Type: BIR/prepared selected proof-edge path record/status API idea
Parent: `ideas/closed/491_dynamic_local_array_selected_proof_edge_path_certificate.md`
Source Evidence:
- `build/agent_state/491_step3_selected_edge_path_route/route.md`
- `build/agent_state/491_step4_residual_disposition/disposition.md`
Owning Layer: Prepared record/status surface for selected proof-edge path certificates keyed to LIR producer coordinates

## Goal

Add a durable prepared record/status API for selected proof-edge path
certificates keyed to dynamic local-array `lir_producer_lookup_key` records.

## Why This Exists

Idea 491 proved that the selected proof-edge path certificate cannot be
implemented soundly by consuming raw branch labels or helper-local
reachability/dominance queries directly. Downstream proof population needs an
explicit prepared surface that records selected outcome, edge tuple, path
coverage, dominance/guard validity, and fail-closed statuses for each relevant
`lir_producer_lookup_key`.

## In Scope

- Define a prepared record keyed by all `lir_producer_*` fields and
  `lir_producer_lookup_key`.
- Carry prepared proof branch/compare identity, dynamic-index operand role, and
  bound contribution when available.
- Represent explicit selected outcome (`true` or `false`) and edge tuple:
  proof branch label, selected successor label, and non-selected successor
  label.
- Represent path coverage from selected successor to LIR producer block.
- Represent dominance or guard validity under the selected outcome.
- Represent fail-closed statuses for missing proof source, missing selected
  edge/outcome, non-covering path, non-dominating/non-guarding proof,
  unsupported boundary, missing same-block ordering, prepared/BIR coordinate
  confusion, raw-shape-only inference, and target/final-home-only inference.
- Add focused prepared/backend contract coverage for available and unavailable
  records if an implementation packet is selected.

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

- Audit identifies the exact prepared record/status home and existing helper
  inputs that can feed it, or records the next lower API blocker.
- Contract names every field required by the 491 route disposition and defines
  unavailable statuses.
- If implementation is selected, prepared dumps/tests expose available records
  and fail-closed unavailable statuses without changing downstream proof
  population or checker inputs.
- Residual disposition states whether selected proof-edge path certification
  can resume, or whether another lower producer/API owner remains first.

## Reviewer Reject Signals

Reject patches that:

- infer selected proof edges or path coverage from raw branch shape, value
  names, block labels, testcase names, dump order, final homes, or target
  behavior;
- consume namespace-local reachability/dominance helper results downstream
  without publishing explicit record/status facts;
- mark path facts available without keying the record to the exact
  `lir_producer_lookup_key`;
- treat `lir_producer_instruction_index` as a prepared traversal or BIR
  instruction index;
- combine the record/status API with interval effect/no-clobber classification,
  idea 489 proof population, idea 486 checker input population, idea 484
  packaging, scalar local-load consumption, or RV64/MIR lowering;
- weaken unsupported diagnostics, expectations, allowlists, pass/fail
  accounting, runtime comparison, or baseline logs as proof of progress.
