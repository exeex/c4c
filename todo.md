Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Broader Backend Validation follow-up repair

# Current Packet

## Just Finished

Step 4 (`Broader Backend Validation follow-up repair`) fixed the
store-source producer metadata agreement regression exposed by the broader
backend subset.

Changed files:
- `src/backend/prealloc/publication_plans.cpp`
- `tests/backend/mir/backend_store_source_publication_plan_test.cpp`
- `todo.md`
- `test_after.log`

Completed work:
- Kept home-backed producer metadata publication fail-closed on complete
  source-home/stored-value agreement.
- Added the semantic byval load-local authority path so prepared byval source
  producers can publish when their payload result exactly matches the stored
  source value.
- Aligned focused direct-helper compatibility assertions with their prepared
  block labels while preserving different stored-value surfaces where relevant.

## Suggested Next

After supervisor review and commit, call the plan owner to decide whether this
one-candidate runbook is ready for close review against the source idea.

## Watchouts

- The byval path still requires the prepared byval classifier, a concrete
  `LoadLocal` payload, a producer before the destination store, matching
  producer/source value name and type, and the existing same-block check.
- Route-only or payload-incomplete producer metadata remains unpublished.

## Proof

Ran delegated proof:
`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_store_source_publication_plan)$'; } > test_after.log 2>&1`

Result: passed. Proof log: `test_after.log`.

Supervisor broader validation:
`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed.
