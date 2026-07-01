# Dynamic Local-Array Selected Proof-Edge Path Certificate

Status: Closed
Type: Routed blocker investigation
Parent: `ideas/closed/490_dynamic_local_array_lir_producer_path_no_clobber_certificate.md`
Closed By: lifecycle split to `ideas/open/492_dynamic_local_array_selected_proof_edge_path_record_status_api.md`
Source Evidence:
- `build/agent_state/490_step3_certificate_producer_route/route.md`
- `build/agent_state/490_step4_residual_disposition/disposition.md`
- `build/agent_state/491_step3_selected_edge_path_route/route.md`
- `build/agent_state/491_step4_residual_disposition/disposition.md`
Owning Layer: BIR/prepared selected proof-edge path certificate route

## Closure Summary

Idea 491 is complete as a routed blocker investigation, not as an implemented
certificate producer.

The runbook established the selected proof-edge path certificate contract and
proved that current prepared/BIR surfaces cannot implement it without a new
durable record/status API keyed to `lir_producer_lookup_key`.

## Completed Work

- Audited prepared branch/compare records, true/false successor labels,
  helper-level reachability/dominance logic, `lir_producer_*` keys,
  same-block ordering boundaries, and status vocabulary.
- Defined the selected-edge path certificate key, accepted cross-block shape,
  selected outcome and edge tuple, path coverage, dominance/guard requirements,
  same-block fail-closed policy, and fail-closed statuses.
- Routed implementation because no durable selected proof-edge path
  record/status API exists.

## Follow-Up

The next active owner is:

`ideas/open/492_dynamic_local_array_selected_proof_edge_path_record_status_api.md`

That follow-up owns the exact missing API surface:

- all `lir_producer_*` fields and `lir_producer_lookup_key`;
- prepared proof branch/compare identity;
- dynamic-index operand role and bound contribution when available;
- explicit selected outcome, `true` or `false`;
- edge tuple: proof branch label, selected successor label, non-selected
  successor label;
- path coverage from selected successor to LIR producer block;
- dominance or guard validity under the selected outcome;
- statuses for missing proof source, missing selected edge/outcome,
  non-covering path, non-dominating/non-guarding proof, unsupported boundary,
  missing same-block ordering, prepared/BIR coordinate confusion,
  raw-shape-only, and target/final-home-only.

## Preserved Boundaries

- Dynamic-index interval effect and no-clobber classification remains a later
  owner after selected proof-edge path records exist.
- Idea 489 proof population remains blocked until both selected path
  certification and interval/no-clobber facts are available.
- Idea 486 checker inputs must not be populated directly from this route.
- Idea 484 packaging, scalar local-load consumption, and RV64/MIR lowering
  remain out of scope.
- Same-block candidates remain unavailable until a truthful LIR-to-prepared/BIR
  ordering bridge exists.

## Validation

Close/split lifecycle validation:

```sh
git diff --check
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed
python3 scripts/plan_review_state.py show
```

Result: passed. Regression sanity used the unchanged canonical log as a
lifecycle-only close gate (`328/328` before and after).

## Reviewer Reject Signals

Reject attempts to reopen this source as progress if they:

- infer selected proof edges or path coverage from branch proximity, loop
  shape, value names, testcase names, dump order, final homes, or RV64 target
  behavior;
- treat helper-local reachability/dominance queries as durable proof facts
  without publishing explicit records or statuses;
- mark path facts available without a record keyed to the exact
  `lir_producer_lookup_key`;
- treat `lir_producer_instruction_index` as a prepared traversal or BIR
  instruction index;
- mix in dynamic-index interval effect/no-clobber classification, idea 489
  proof population, idea 486 checker input population, idea 484 packaging,
  scalar local-load consumption, or RV64/MIR lowering;
- weaken unsupported diagnostics, expectations, allowlists, pass/fail
  accounting, runtime comparison, or baseline logs as proof of progress.
