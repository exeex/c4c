# Current Packet

Status: Active
Source Idea Path: ideas/open/705_prepared_fact_boundary_from_bir_views.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate prepared publication production

## Just Finished

- Step 2 — classified current-block edge publication facts by semantic origin
  (`BirPhi` versus prepared `JoinTransfer`) and made named evidence applicability
  explicit. A non-PHI edge is accepted only with complete unique prepared
  transfer authority and matching named producer evidence; incomplete prepared
  authority and missing, ambiguous, or mismatched applicable evidence fail
  closed.

## Suggested Next

- Continue Step 2 with the next prepared publication-production seam selected
  by the supervisor.

## Watchouts

- `backend_prepared_lookup_helper_test` becomes available when configured with
  `C4C_ENABLE_PREPARED_FACT_TESTS=ON`, but a direct focused compile currently
  fails later on pre-existing undeclared `route1_query` / `route1_index`
  errors. The new focused case therefore could not execute in that target.
- Production query-input callsites now pass their existing
  `PreparedControlFlowFunction`; a null control-flow input remains
  intentionally fail-closed for non-PHI prepared publication authority.
- Do not widen into call plans, prepared lookups, target materializers, or the
  common MIR migration owned by idea 706.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
- Result: passed, 309/309 backend tests; canonical proof log:
  `test_after.log`.
- Supervisor focused compile: configured prepared-fact tests exposed
  `backend_prepared_lookup_helper_test`, but compilation stopped on the
  pre-existing undeclared `route1_query` / `route1_index` errors before the new
  case could run.
