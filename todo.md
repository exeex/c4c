# Current Packet

Status: Active
Source Idea Path: ideas/open/706_common_mir_named_query_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Audit ownership and run integration proof — compile-repair prerequisite

## Just Finished

- Step 6 compile-repair prerequisite adapted the x86 decoded-home fixture to
  construct and pass the ownership-correct `PreparedMirFunctionView`, publish
  matching structured block/control-flow facts, and test prepared-MIR authority
  independently of the retired consumed-lookup route.
- Repaired the prepared lookup fixture's dangling local Route 1 query setup and
  immutable block-label reset so the full default build now compiles all targets.

## Suggested Next

- Continue Step 6 with a bounded runtime-baseline triage packet beginning at
  `backend_prepared_lookup_helper` (`block-index label bridge should accept
  agreeing structured ids`), then classify the other 23 broad-backend failures
  against the matching canonical baseline before ownership acceptance proceeds.

## Watchouts

- The compile gap is closed and `backend_x86_prepared_decoded_home_storage`
  passes. The broad backend run still has 24 runtime failures; no runtime fixes
  outside the delegated compile-repair fixture scope were attempted.
- `backend_prepared_lookup_helper` now compiles but stops at its earlier
  block-index label bridge assertion before reaching the repaired late fixture.
- The broad failures include the known frame/call contract failure plus prepared
  printer/layout/liveness, one x86 handoff abort, RV64 dumps, and prepared-BIR CLI
  contracts; Step 6 cannot proceed to acceptance without baseline comparison.

## Proof

- Exact delegated command: `{ cmake --build --preset default && ctest
  --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log
  2>&1`. The default build succeeds; 373/397 backend tests pass and 24 fail.
  Canonical proof log: `test_after.log`.
