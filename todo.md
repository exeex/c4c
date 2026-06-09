Status: Active
Source Idea Path: ideas/open/145_current_block_join_fact_routing_split.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Split Value-home and Out-of-SSA Predicate Roles

# Current Packet

## Just Finished

Step 4 confirmed no additional code split was needed after Step 3. The reusable
value-home/out-of-SSA predicates remain in shared prealloc as source/dataflow
facts:
`prepared_value_homes_share_register_name`,
`prepared_out_of_ssa_parallel_copy_register_destination_matches_value`, and
`prepared_out_of_ssa_parallel_copy_source_shares_destination_register`.

AST-backed checks confirmed
`prepare_current_block_join_parallel_copy_source_facts` is the shared owner of
those facts, calling the out-of-SSA predicates while populating source fact
fields. Cross-file search and AArch64 TU caller checks confirmed the target
routing adapters stay in AArch64 code:
`build_current_block_join_prepared_query_routing` consumes only the shared
`prepare_current_block_join_parallel_copy_source_facts` query,
`dispatch_prepared_block` calls the AArch64 routing helpers, and
`emit_value_publication_to_register` calls the AArch64
`prepared_query_current_block_join_parallel_copy_source` adapter.

## Suggested Next

Proceed to Step 5 by validating and preparing the supervisor handoff for the
owner split.

## Watchouts

- `dispatch_producers.cpp` intentionally keeps per-instruction routing vectors
  target-local; shared prealloc exposes only reusable value-home/out-of-SSA and
  source-fact predicates.
- The prealloc helper test verifies the shared predicates/facts only. AArch64
  routing behavior is covered through the backend/AArch64 dispatch tests in the
  delegated proof.
- The pre-existing untracked `review/145_step2_route_review.md` was not touched.

## Proof

Ran:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

Result: passed. `ctest` reported 179/179 backend tests passed.
Regression guard passed with 179/179 backend tests before and after, no new
failures.
Proof log: accepted after-proof was rolled forward to `test_before.log`;
there is no current root `test_after.log`.
