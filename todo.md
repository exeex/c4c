Status: Active
Source Idea Path: ideas/open/145_current_block_join_fact_routing_split.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Localize AArch64 Routing Convenience

# Current Packet

## Just Finished

Step 3 localized the remaining current-block join parallel-copy instruction
routing convenience into AArch64 dispatch producer code. The shared
`PreparedCurrentBlockJoinParallelCopyInstructionRouting` type and
`prepare_current_block_join_parallel_copy_instruction_routing` function were
removed from `prepared_lookups`, while AArch64 now builds its
`CurrentBlockJoinPreparedQueryRouting` vectors directly from
`prepare_current_block_join_parallel_copy_source_facts`.

AST-backed checks confirmed `build_current_block_join_prepared_query_routing`
now calls the shared source-fact query from `publication_plans.hpp`, and that
the removed shared routing function no longer has a declaration in
`prepared_lookups.cpp`.

## Suggested Next

Proceed to Step 4 by reviewing the remaining value-home/out-of-SSA predicate
roles and separating reusable facts from any AArch64 routing-only use.

## Watchouts

- `dispatch_producers.cpp` intentionally keeps per-instruction routing vectors
  target-local; shared prealloc exposes only the source-fact query.
- The prealloc helper test now verifies the shared source facts only. AArch64
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
