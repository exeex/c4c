# Dynamic Local-Array Selected Proof-Edge Path Record/Status API

Status: Closed
Type: Completed BIR/prepared record/status API surface
Parent: `ideas/closed/491_dynamic_local_array_selected_proof_edge_path_certificate.md`
Closed By: lifecycle split to `ideas/open/493_dynamic_local_array_selected_proof_edge_path_record_collector_population.md`
Source Evidence:
- `build/agent_state/491_step3_selected_edge_path_route/route.md`
- `build/agent_state/491_step4_residual_disposition/disposition.md`
- `build/agent_state/492_step3_selected_proof_edge_record_api/summary.md`
- `build/agent_state/492_step4_residual_disposition/disposition.md`
Owning Layer: Prepared record/status surface for selected proof-edge path certificates keyed to LIR producer coordinates

## Closure Summary

Idea 492 is complete for the independent selected proof-edge path record/status
API surface.

Step 3 added the bounded BIR local-array metadata home and API-specific
fail-closed vocabulary required by the 491 route. Step 4 classified the
remaining first owner as real collector/population work over the new API, not
another API-surface packet.

## Completed Work

- Added `bir::Function::local_array_selected_proof_edge_paths` as the durable
  BIR local-array metadata home.
- Added selected proof-edge path record inputs, records, statuses, evaluator,
  and stringifiers.
- Added keying fields for local-array paths, `lir_producer_*`, and
  `lir_producer_lookup_key`.
- Added proof identity, selected outcome, edge tuple, path coverage, and
  dominance/guard fields.
- Added focused backend/BIR tests for explicit available records, durable
  storage, and fail-closed statuses.

## Follow-Up

The next active owner is:

`ideas/open/493_dynamic_local_array_selected_proof_edge_path_record_collector_population.md`

That follow-up owns real collector/population work for:

- exact `lir_producer_lookup_key` matches to local-array element path records;
- available LIR producer coordinate status and `address_derivation` role;
- proof function identity matching the LIR producer function;
- prepared proof branch/compare identity;
- explicit selected outcome and selected/non-selected successor edge tuple;
- path coverage from selected successor to the LIR producer block;
- dominance or guard validity under the selected outcome;
- unavailable records for missing/mismatched path, key, coordinate, role, proof
  source, selected edge/outcome, non-covering path, non-guarding proof,
  same-block ordering gap, unsupported boundary, coordinate confusion,
  raw-shape-only evidence, and target/final-home-only evidence.

Printer/display exposure is optional and should remain display-only unless the
collector owner needs it for durable probe evidence. It is not the first
semantic blocker.

## Preserved Boundaries

- Dynamic-index same-value and no-clobber interval classification remains a
  later owner after selected proof-edge path records are available.
- Idea 489 proof facts must not be populated directly by this API slice.
- Idea 486 checker inputs remain downstream consumers and must wait for
  explicit available producer facts.
- Idea 484 packaging, scalar local-load consumption, and RV64/MIR lowering
  remain out of scope.
- Same-block ordering remains unavailable unless a later bridge truthfully
  relates LIR producer coordinates to the relevant execution coordinate.

## Validation

Close/split lifecycle validation:

```sh
git diff --check
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed
python3 scripts/plan_review_state.py show
```

Result: passed. `test_after.log` was absent and logs were out of scope for this
lifecycle-only delegation, so regression sanity used the unchanged canonical
`test_before.log` as both inputs (`328/328`).

## Reviewer Reject Signals

Reject attempts to reopen this API source as progress if they:

- infer selected proof edges or path coverage from raw branch shape, value
  names, block labels, testcase names, dump order, final homes, or target
  behavior;
- consume helper-local reachability/dominance facts downstream without
  producing explicit records/statuses;
- mark proof facts or checker inputs available directly from this API surface;
- combine record API maintenance with collector/population, interval
  effect/no-clobber classification, idea 489 proof population, idea 486 checker
  input population, idea 484 packaging, scalar local-load consumption, or
  RV64/MIR lowering;
- weaken unsupported diagnostics, expectations, allowlists, pass/fail
  accounting, runtime comparison, or baseline logs as proof of progress.
