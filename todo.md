Status: Active
Source Idea Path: ideas/open/60_aarch64_dispatch_lookup_wrapper_fold_back.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Fold Publication Result-Value Helpers

# Current Packet

## Just Finished

Step 4 - Fold Publication Result-Value Helpers completed the assigned
publication result-value sub-slice: removed the public
`instruction_result_value` and `instruction_result_value_ref` declarations and
definitions from `dispatch_publication.hpp`/`.cpp`.

The remaining consumers in `dispatch.cpp`, `dispatch_producers.cpp`,
`memory.cpp`, and `cast_ops.cpp` now use private anonymous-namespace extraction
convenience local to the immediate owning file. The extraction logic preserves
the previous result facts for binary, cast, select, load-local, load-global,
and optional call results.

## Suggested Next

Continue with supervisor-selected Step 5 public surface pruning and acceptance
proof for the dispatch lookup, producer, and publication fold-back slice.

## Watchouts

The generic result extraction remains mechanical and private to its local
consumers; no publication result behavior, comparison hook fact, FP
materialization fact, or dispatch hook condition was intentionally changed.
There is still no shared MIR query for generic instruction result extraction,
so this slice did not move authority into `src/backend/mir/query.*`.

## Proof

Ran and passed:
`(cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`.

Result: build succeeded; `ctest -R '^backend_'` passed 169/169 tests. Proof log:
`test_after.log`.
