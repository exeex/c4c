Status: Active
Source Idea Path: ideas/open/697_bir_memory_publication_view_extraction.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Prove Publication Boundary Preservation

# Current Packet

## Just Finished

Completed the `plan.md` Step 2-5 Route 4-only publication-view slice. Added
`bir::BirPublicationView`, `bir::make_bir_publication_view(function)`, and
`bir::validate_current_block_publication_reference(view, block, value,
before_instruction_index)`. The named view keeps the Route 4 availability
index private and delegates to the existing Route 4 current-block validation
path, returning the same `Route4IndexReferenceValidation` payload.

Migrated only
`tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`
`check_call_argument_source_producer_materializability_contract` for the
focused `%sum` current-block proof. The valid `%sum`, `%missing`, wrong-type,
duplicate, stale-owner, and wrong-relationship checks now use the named
publication view instead of direct Route 4 validation calls.

Route 5 remains deferred. No Route 5 implementation, production
MIR/prealloc/codegen files, expectations, unsupported markers, allowlists,
timeouts, docs, runtime contracts, default harness contracts, or root logs
other than `test_after.log` were changed.

Rollback point: restore the local `route4_index` construction plus direct
`route4_validate_current_block_publication_reference` calls in
`check_call_argument_source_producer_materializability_contract`, then remove
the `BirPublicationView` declarations/definitions from `bir.hpp` and
`bir_route4_publication.cpp`.

## Suggested Next

After this slice is committed, request plan-owner lifecycle review. The active
runbook's Route 4-only publication-view adapter, first proof-reader migration,
Route 5 deferral, and publication-boundary proof are complete; any additional
publication-view, Route 5, block-entry, or production consumer migration should
be a follow-up lifecycle decision.

## Watchouts

The duplicate and wrong-relationship negative states are now produced through
small BIR functions and the named view, not by mutating exposed Route 4 index
records. This keeps the adapter behavior-preserving without adding
testcase-shaped matching, expectation rewrites, unsupported-marker edits, or
Route 5 expansion.

## Proof

Supervisor baseline proof passed before delegation with:
`set -o pipefail; (cmake --build --preset default && build/tests/backend/bir/backend_prepare_frame_stack_call_contract_test) > test_before.log 2>&1`

Implementation proof passed after the slice with:
`set -o pipefail; (cmake --build --preset default && build/tests/backend/bir/backend_prepare_frame_stack_call_contract_test) > test_after.log 2>&1`

Proof logs: `test_before.log` and `test_after.log`.
