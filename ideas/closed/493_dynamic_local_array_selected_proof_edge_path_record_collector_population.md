# Dynamic Local-Array Selected Proof-Edge Path Record Collector/Population

Status: Closed
Type: Completed selected proof-edge path collector/population slice
Parent: `ideas/closed/492_dynamic_local_array_selected_proof_edge_path_record_status_api.md`
Closed By: lifecycle split to `ideas/open/494_dynamic_local_array_lir_producer_interval_effect_classifier.md`
Source Evidence:
- `build/agent_state/492_step3_selected_proof_edge_record_api/summary.md`
- `build/agent_state/492_step4_residual_disposition/disposition.md`
- `build/agent_state/493_step3_collector_population/summary.md`
- `build/agent_state/493_step4_residual_disposition/disposition.md`
Owning Layer: Collector/population for selected proof-edge path records keyed to LIR producer coordinates

## Closure Summary

Idea 493 is complete as the real selected proof-edge path
collector/population slice.

Step 3 implemented the missing collector that idea 492 left open:
`BirPreAlloc::publish_contract_plans()` now populates
`bir::Function::local_array_selected_proof_edge_paths` from real prepared
branch/control-flow facts and matching dynamic local-array
`lir_producer_lookup_key` records.

## Completed Work

- Populated real `local_array_selected_proof_edge_paths` during prepared
  contract publication.
- Matched records by prepared function spelling plus exact
  `lir_producer_lookup_key`.
- Accepted prepared fused branch compares with predicate/type/lhs/rhs and
  true/false successor labels.
- Recorded dynamic-index operand role, immediate bound contribution, selected
  successor edge, path coverage, and dominance/guard facts.
- Published helper-derived reachability/dominance facts as durable
  record/status fields instead of consuming helper-local results downstream.
- Kept same-block proof/producer candidates fail-closed as
  `missing_same_block_ordering`.
- Added focused backend coverage for available cross-block records and
  fail-closed collector statuses.

## Follow-Up

The next active owner is:

`ideas/open/494_dynamic_local_array_lir_producer_interval_effect_classifier.md`

That follow-up owns the remaining producer family required before full proof
population can resume:

- identity of the dynamic index value used by the local-array element path and
  proof compare;
- selected proof branch/edge and `lir_producer_lookup_key` binding from the
  populated selected proof-edge record;
- interval start/end from proof source to LIR producer site without treating
  `lir_producer_instruction_index` as a prepared/BIR instruction index;
- same-value proof for the dynamic index over that interval;
- effect classification for redefinitions/assignments, phi/alias edges,
  calls/helpers, inline asm, publications, move bundles, and parallel copies;
- fail-closed statuses for clobbered/redefined index, unresolved alias,
  unknown effect, missing path/order, raw-shape-only inference,
  target/final-home-only inference, and prepared/BIR coordinate confusion.

## Preserved Boundaries

- Idea 489 proof facts must not be populated directly from 493 records alone;
  they require selected path facts plus interval/no-clobber facts.
- Idea 486 checker inputs remain downstream consumers and should only consume
  explicit available producer facts from later proof-population work.
- Idea 484 packaging remains blocked for dynamic local-array rows until proof
  population can produce available range/provenance facts.
- Scalar local-load work and RV64/MIR lowering remain out of scope.
- Same-block LIR-to-prepared/BIR instruction ordering conversion remains a
  separate unavailable boundary.

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

Reject attempts to reopen this collector source as progress if they:

- infer selected proof-edge records from testcase names, block labels, value
  names, dump order, final homes, or RV64 target behavior;
- bypass the populated records and consume helper-local reachability/dominance
  facts in downstream proof population;
- match records by fuzzy branch shape instead of exact function and
  `lir_producer_lookup_key` identity;
- combine collector maintenance with interval effect/no-clobber
  classification, idea 489 proof population, idea 486 checker input
  population, idea 484 packaging, scalar local-load consumption, or RV64/MIR
  lowering;
- weaken unsupported diagnostics, expectations, allowlists, pass/fail
  accounting, runtime comparison, or baseline logs as proof of progress.
