# Dynamic Local-Array Selected Proof-Edge Path Certificate

Status: Open
Type: BIR/prepared selected proof-edge path certificate producer idea
Parent: `ideas/closed/490_dynamic_local_array_lir_producer_path_no_clobber_certificate.md`
Source Evidence:
- `build/agent_state/490_step3_certificate_producer_route/route.md`
- `build/agent_state/490_step4_residual_disposition/disposition.md`
Owning Layer: BIR/prepared selected proof-edge and path coverage certificate keyed to LIR producer coordinates

## Goal

Publish a durable selected proof-edge path certificate keyed to dynamic
local-array `lir_producer_*` records so later interval-effect and proof
population work can consume explicit path facts instead of inferring from raw
shape.

## Why This Exists

Idea 490 proved the larger LIR producer path/no-clobber certificate cannot be
implemented until a lower producer certifies the selected proof edge and path
from a prepared proof branch/compare to the exact LIR producer site. Current
surfaces expose candidate branch/compare facts and `lir_producer_*` keys, but
no durable record selects the proof edge/outcome or proves path coverage,
dominance/guard validity, and same-block ordering safety for that key.

## In Scope

- Audit prepared branch/compare proof sources and dynamic local-array
  `lir_producer_*` path records for selected-edge certificate inputs.
- Define certificate keys for function, LIR producer block/index/role/key,
  proof branch/compare identity, dynamic-index operand role, bound
  contribution, and selected edge/outcome.
- Publish path coverage from proof source to `lir_producer_*` site when the
  selected edge truthfully covers the producer site.
- Publish dominance or guard validity under the selected outcome when supported.
- Fail closed for missing selected edge, missing path, non-covering path,
  non-dominating/non-guarding proof, unsupported boundary, missing same-block
  ordering, and prepared/BIR coordinate confusion.

## Out Of Scope

- Dynamic-index same-value/no-clobber interval effect classification.
- Populating idea 489 proof facts or idea 486 checker inputs.
- Idea 484 packaging.
- Scalar local-load consumption.
- RV64/MIR lowering.
- Prepared traversal/BIR instruction coordinate conversion beyond explicit
  fail-closed status reporting.
- Broad generic range analysis.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, runtime-comparison changes, or baseline/log churn.

## Acceptance Criteria

- Focused audit identifies the current prepared/BIR sources for selected proof
  edge/outcome and path coverage, or records the exact lower blocker.
- The certificate contract names proof source, selected outcome, LIR producer
  key, path coverage, dominance/guard validity, same-block ordering policy, and
  fail-closed statuses.
- If implementation is selected, positive coverage proves at least one real
  dynamic local-array proof source has an available selected proof-edge path
  certificate keyed to `lir_producer_*`.
- Negative coverage rejects raw-shape-only, missing-edge, non-covering,
  non-dominating/non-guarding, unsupported-boundary, same-block-ordering, and
  coordinate-confusion cases.
- Residual disposition states whether the later dynamic-index interval effect
  classifier can proceed or whether another selected-edge/path lower owner
  remains first.

## Reviewer Reject Signals

Reject patches that:

- infer selected proof edges or path coverage from branch proximity, loop shape,
  value names, testcase names, dump order, final homes, or RV64 target behavior;
- treat a helper-local reachability/dominance query as a durable certificate
  without publishing explicit prepared records or statuses;
- mark path facts available without a certificate keyed to the exact
  `lir_producer_lookup_key`;
- treat `lir_producer_instruction_index` as a prepared traversal or BIR
  instruction index;
- mix in dynamic-index interval effect/no-clobber classification, idea 489
  proof population, idea 486 checker input population, idea 484 packaging,
  scalar local-load consumption, or RV64/MIR lowering;
- weaken unsupported diagnostics, expectations, allowlists, pass/fail
  accounting, runtime comparison, or baseline logs as proof of progress.
