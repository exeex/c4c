Status: Active
Source Idea Path: ideas/open/696_bir_producer_index_view_extraction.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Prove Behavior Preservation

# Current Packet

## Just Finished

Implemented the `plan.md` Step 2-5 code slice in the owned files. Added a
Route 1-only `bir::BirProducerView` surface with
`bir::make_bir_producer_view`, `bir::find_same_block_scalar_producer`, and
`bir::find_materialization_availability`; the named functions delegate to the
existing Route 1 producer index/query path and return the same
`Route1SameBlockScalarProducer` and `Route1MaterializationAvailability`
payloads.

Migrated the selected `%sum` Route 1 proof reader in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp` through the named
view. The current source places that `%sum` proof in
`verify_prepared_same_block_scalar_source_facts`; the named
`verify_prepared_semantic_name_agreement_boundary` function contains only
semantic name-agreement checks and no `%sum` Route 1 reader to migrate.

Route 2 remains deferred. No Route 2 implementation, production
MIR/codegen/prealloc files, expectations, unsupported markers, allowlists,
timeouts, docs, or non-`test_after.log` root logs were changed.

Rollback point: restore the local `route1_index`, `route1_query`,
`route1_find_same_block_scalar_producer`, and
`route1_find_materialization_availability` calls in the `%sum` proof reader,
then remove the `BirProducerView` declarations/definitions from `bir.hpp` and
`bir.cpp`.

## Suggested Next

After this slice is committed, request plan-owner lifecycle review. The active
runbook's Route 1-only producer-view adapter, first proof-reader migration,
Route 2 deferral, and behavior-preservation proof are complete; any additional
producer-view or Route 2 migration should be a follow-up lifecycle decision.

## Watchouts

The implementation is intentionally a compatibility adapter, not a semantic
rewrite: it delegates to Route 1 and keeps Route 1 storage private inside the
view. No testcase-shaped shortcuts, expectation rewrites, unsupported-marker
edits, or Route 2 expansion were used.

## Proof

The delegated proof command used a stale Ninja target name and failed before
compile. The supervisor regenerated a true before/after pair with the corrected
focused proof shape by stashing this slice, running:
`set -o pipefail; (cmake --build --preset default && build/tests/backend/bir/backend_prepared_lookup_helper_test) > test_before.log 2>&1`

then restoring this slice and running:
`set -o pipefail; (cmake --build --preset default && build/tests/backend/bir/backend_prepared_lookup_helper_test) > test_after.log 2>&1`

Both corrected proof commands passed.

Stale delegated command retained for diagnosis:
`set -o pipefail; (cmake --build --preset default --target backend_prepared_lookup_helper_test && build/tests/backend/bir/backend_prepared_lookup_helper_test) > test_after.log 2>&1`

Stale-command result: blocked before compile. Ninja reported:
`unknown target 'backend_prepared_lookup_helper_test'`.

Proof logs: `test_before.log` and `test_after.log`.
