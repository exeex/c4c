Status: Active
Source Idea Path: ideas/open/146_call_argument_materialization_call_owner.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Update Consumers and Includes

# Current Packet

## Just Finished

Completed Step 4 consumer include routing for the call-owned materialization API.

- Direct test consumer
  `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp` now
  includes `src/backend/prealloc/calls.hpp` where it names
  `find_prepared_call_argument_source_producer_materialization`.
- Existing direct implementation consumers already included `calls.hpp`.
- No `prepared_lookups.hpp` include was removed because the owned files that
  still include it have unrelated prepared lookup dependencies.

## Suggested Next

Next packet: supervisor-selected Step 5 validation/lifecycle handoff for
deciding whether this runbook is exhausted or needs a separate residual include
cleanup idea.

## Watchouts

- AST checks confirmed
  `find_prepared_call_argument_source_producer_materialization` declares from
  `src/backend/prealloc/calls.hpp` and defines from
  `src/backend/prealloc/call_plans.cpp`.
- `src/backend/mir/aarch64/codegen/calls.cpp` still uses prepared lookup state,
  current-block publication consumption, and other shared lookup helpers; its
  `prepared_lookups.hpp` include is not solely for the moved API.
- `src/backend/prealloc/call_plans.cpp` still uses shared producer lookup
  types/helpers; its `prepared_lookups.hpp` include is not solely for the moved
  API.
- Do not start broad residual prepared-lookup include cleanup inside this
  ownership route.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
passed with 179/179 backend tests green. Regression guard passed with 179/179
backend tests before and after, no new failures.
Proof log: accepted after-proof was rolled forward to `test_before.log`;
there is no current root `test_after.log`.
