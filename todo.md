# Execution State

Status: Active
Source Idea Path: ideas/open/64_prepared_cfg_authoritative_completion_and_parallel_copy_correctness.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Complete Prepared Control-Flow Publication
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Completed Step 2's prepared join-ownership slice by moving branch-owned join
predecessor discovery onto shared `PreparedControlFlowFunction.blocks`
publication facts instead of raw-BIR linear predecessor rediscovery, and
extended backend prepare coverage to prove the published join ownership can be
recovered directly from prepared control-flow metadata, including a dedicated
`backend_prepare_authoritative_join_ownership` target that makes the slice
acceptance-ready under the monotonic regression guard.

## Suggested Next

Take the next packet on Step 3 so `SaveDestinationToTemp` stops being a
plan-only marker and regalloc move resolution gains an explicit consumer-facing
temporary-save contract for cyclic parallel-copy bundles.

## Watchouts

- Do not patch one failing CFG or phi shape at a time.
- Keep this route in shared prepare/control-flow publication rather than
  target-specific emitter cleanup.
- Step 2 now publishes block-only control-flow and branch-owned join mapping
  from prepared facts, but Step 3 still owns the missing cycle temp-save
  semantics in `regalloc.cpp`.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_'` and captured output in `test_after.log`.
The command still exits non-zero because the same carried backend-route tests
34, 35, 76, and 77 fail in `test_after.log`, but the monotonic guard passed
against `test_before.log` at `69/73 -> 70/74` with no new failures after
adding and passing `backend_prepare_authoritative_join_ownership`.
