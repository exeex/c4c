# Route Debug And Test Vocabulary Cleanup

Status: Open
Type: backend proof and fixture cleanup
Parent: `ideas/open/703_bir_mir_contract_abstraction_umbrella.md`
After:
- `ideas/open/708_x86_named_handoff_materializer_cleanup.md`
- `ideas/open/709_aarch64_named_handoff_materializer_cleanup.md`
- `ideas/open/710_rv64_named_handoff_materializer_cleanup.md`
- `ideas/open/711_bir_route_implementation_quarantine.md`

## First Owner And Scope

First owning layer: backend debug/proof and tests.  Replace direct route API
fixtures, route-labelled dumps, agreement vocabulary, and CMake test names
with named producer/prepared contract proof after semantic consumers migrate.

First migrated consumer: direct route fixtures in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`, followed by the
remaining guarded BIR/MIR fixtures and x86/RV64 debug labels.

## Dependencies And Proof

- Depends on all target migrations and route quarantine.
- Proof surface: affected backend BIR/MIR suites plus a guard proving route
  vocabulary is absent outside explicitly accepted private BIR internals.

## Retirement Guard

The umbrella guard must reach its final state: zero hits in prealloc, MIR,
targets, and tests, with only reviewed private BIR implementation hits if any.

## Acceptance Criteria

- Public fixtures construct named inputs or exercise real producers.
- Dumps describe ownership-named agreement/rejection facts and never authority.
- Test coverage remains equivalent or stronger while old route fixture and
  label vocabulary disappears.

## Reviewer Reject Signals

- Expectation-only renames stand in for consumer migration.
- Unsupported markers, allowlists, runtime behavior, harness policy, or proof
  breadth are weakened.
- A direct route fixture survives because production code no longer exercises
  the same contract.
