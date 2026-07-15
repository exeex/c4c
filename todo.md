# Current Packet

Status: Active
Source Idea Path: ideas/open/754_lir_aggregate_vector_value_identity_convergence.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit and select one aggregate/vector authority row

## Just Finished

- Step 1 audit selected `LirExtractValueOp` as the only authority row for
  this runbook.  Its Step 2 schema must require a current-function SSA
  `result` `LirValueId` and, when `agg` is SSA, a current-function aggregate
  use `LirValueId`; `agg_type` is the closed-763 `LirTypeRef` composite
  carrier and `index` is the exact non-negative aggregate field index.  The
  display spellings in `result.str()` and `agg.str()` remain printer mirrors
  only and may not supply or repair an ID, type, or index.
- `LirInsertValueOp` is deferred because it adds independent element-type and
  aggregate/result coherence; `LirInsertElementOp` and `LirExtractElementOp`
  are deferred because their lane/index-type contracts are larger; and
  `LirShuffleVectorOp` is deferred because it adds two input vectors plus a
  mask-type/mask-shape contract.  All four remain fail-closed and receive no
  partial authority fields in this route.
- Nearby positive target: the existing
  `make_rv64_anonymous_aggregate_return_extractvalue_module` /
  `expect_rv64_anonymous_aggregate_return_extractvalue_publishes_lane_facts`
  fixture in `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`.  Nearby
  malformed target for Steps 2-3: a copy of that structured fixture must
  reject a missing/invalid/foreign `result` or SSA `agg` ID, an incoherent
  composite `agg_type`, and a negative/out-of-range `index`, without consulting
  `%t` display text.

## Suggested Next

- Step 2 only: add the minimum opt-in `LirExtractValueOp` structured result and
  aggregate-use authority, wire its existing HIR producers through
  `fresh_value`, and retain legacy rows as compatibility-only/fail-closed.

## Watchouts

- `verify.cpp` currently checks the selected row's operand kinds and type-ref
  presence and visits its aggregate use, but it does not require native IDs or
  validate the exact field index.  Step 2 must use the existing
  current-function definition/use machinery; Step 3 owns index/type coherence.
- Do not infer identities from `%t` or display text, widen to a family sweep,
  or edit Raw BIR before an accepted one-row producer handoff exists.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log` (Step 1 audit; build plus backend subset; log path `test_after.log`).
