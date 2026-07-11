# Current Packet

Status: Active
Source Idea Path: ideas/open/704_bir_semantic_handoff_views.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add memory and publication semantic views

## Just Finished

- Completed Step 1, including its public/private boundary completion check.
  Added the route-free
  `bir_producer_view.hpp` public contract, moved the existing producer-index
  adapter behind `BirProducerView::Implementation`, and replaced the named
  query payload with explicit available, unavailable, incomplete, and
  ambiguous outcomes plus narrow producer identity/materialization facts.
- Added focused positive/negative producer proof and a source/compile guard for
  forbidden route vocabulary in the named public header.

## Suggested Next

- Execute Step 2 as a bounded memory/publication semantic-view packet, reusing
  the shared status vocabulary without widening either result into prepared
  placement, movement, freshness, or destination authority.

## Watchouts

- `BirViewStatus` currently lives with the first public view; extract it only
  when the next view needs the shared vocabulary.
- The route-backed producer adapter remains private in `bir.cpp`; do not add
  downstream callers to its implementation record.

## Proof

- Passed the supervisor-selected command:
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_x86_shared_producer_query$') 2>&1 | tee test_after.log`.
- Passed focused contract behavior/source guard:
  `ctest --test-dir build --output-on-failure -R '^backend_bir_producer_view_contract$'`.
- Supervisor-side broader `^backend_` regression guard passed 304/304 before
  the Step 1 slice and 305/305 after it.
- Canonical proof log: `test_after.log`.
