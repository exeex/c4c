Status: Active
Source Idea Path: ideas/open/93_hir_agent_index_header_hierarchy.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Clarify Inspection Support

# Current Packet

## Just Finished

Step 5 - Move Compile-Time And Follow-Up Transform Boundary tightened the
compile-time/materialization boundary after the first private-index slice.

- Added `src/frontend/hir/impl/compile_time/compile_time.hpp` as the private
  compile-time/materialization implementation index.
- Retargeted `src/frontend/hir/compile_time_engine.cpp` and private HIR
  implementation state to reach compile-time declarations through the new
  private index.
- Documented `compile_time_engine.hpp` as the retained public/app-facing
  compile-time and materialization contract used by c4cll dump/emission paths.
- Documented `inline_expand.hpp` as a separate public follow-up transform
  contract; it should not move under compile_time in this slice.
- Confirmed `src/frontend/hir/impl/compile_time/compile_time.hpp` has no
  inline-expansion include; the private compile-time index keeps only the
  explicitly relative compile-time engine include it needs.

## Suggested Next

Next packet: continue Step 5 follow-up only if the supervisor wants a separate
private boundary for inline expansion; otherwise keep inline expansion as a
public follow-up transform contract for this plan.

## Watchouts

- `c4cll.cpp` directly calls `run_compile_time_engine`,
  `materialize_ready_functions`, and `run_inline_expansion`, so the top-level
  compile-time and inline-expansion headers remain public contracts.
- Inline expansion currently reads as a separate post-normalization transform,
  not compile-time/materialization internals; moving it under compile_time would
  blur the boundary unless a later plan step explicitly re-scopes it.
- This slice is include-boundary only; behavior should remain unchanged.

## Proof

`{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^cpp_hir_'; } > test_after.log 2>&1`

Result: passed for the Step 5 compile-time/materialization boundary tightening
slice. Build completed and HIR subset proof passed 71/71 `cpp_hir_*` tests.
Proof log: `test_after.log`.
