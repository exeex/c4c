Status: Active
Source Idea Path: ideas/open/61_aarch64_shared_same_block_materialization_authority.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Update Producer Wrappers And Consumers

# Current Packet

## Just Finished

Step 5 - Update Producer Wrappers And Consumers completed the in-scope
producer-wrapper migration for same-block select consumers.

Completed:

- Removed the non-prepared `find_same_block_select_producer(...)` wrapper and
  its raw `mir::find_same_block_select_producer(...)` fallback from
  `dispatch_producers.*`.
- Made `dispatch_producers.cpp` source-producer lookup require
  `context.function.prepared_lookups` instead of constructing fallback
  source-producer lookups locally.
- Updated `lower_predecessor_select_parallel_copy_sources(...)` to consume
  `find_prepared_same_block_select_producer(...)` for all select recursion
  paths, including the direct-global dependency path.
- Updated focused direct instruction-dispatch fixtures to attach prepared
  function lookups and to intern nested select result names that shared
  source-producer lookup authority now requires.
- Left the explicit non-goals unchanged: direct-global select-chain fallback
  still lives in `select_chain_contains_direct_global_load(...)`, and
  join-copy expression/source classification remains in the existing
  dispatch-producer helpers without migrating its authority in this packet.

## Suggested Next

Supervisor review and commit of the completed Step 5 slice.

## Watchouts

Producer-wrapper select recursion now fails closed when prepared source
producer lookups are absent. Direct unit-test block contexts that exercise
same-block select materialization through producer wrappers must attach
`PreparedFunctionLookups` and intern nested same-block producer result names.
The direct-global select-chain fallback and join-copy classification paths are
intentionally still separate follow-up/non-goal territory for this plan step.

## Proof

`(cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Passed: 169/169 backend tests. Proof log: `test_after.log`.
