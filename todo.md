# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow packet in
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` and
`tests/backend/backend_x86_handoff_boundary_test.cpp` by routing the minimal
compare-against-zero branch helper through
`c4c::backend::prepare::find_prepared_branch_condition()` and adding a
matching ownership check that mutates the raw entry compare while requiring
the emitted x86 to stay anchored to prepared branch metadata.

## Suggested Next

The next small Step 3 packet is to inspect whether any remaining x86
consumer-side helper in `prepared_module_emit.cpp` still depends on raw BIR
control-flow semantics instead of the prepared contract, or else shift from
lookup cleanup into broader validation and a possible plan-owner closeout
check.

## Watchouts

- Do not activate umbrella idea 57 or idea 59 while cleaning this helper
  surface.
- Do not solve coverage gaps with x86 testcase-shaped matcher growth.
- `build_compare_join_continuation()` remains the Step 3 gate for the join-
  result zero-compare contract; keep `Eq`/`Ne` mapping and jump-target choice
  data-driven there instead of pushing them back into renderer assembly.
- Keep `build_short_circuit_join_context_from_transfer()` responsible for join
  discovery, lane classification, and prepared predecessor ownership checks;
  direct-entry helper cleanup must not pull those checks back into render
  paths.
- `CompareDrivenBranchRenderPlan` now owns the compare setup/opcode plus
  branch-lane targets that feed `render_compare_driven_branch_plan()`; future
  cleanup should extend that contract instead of re-splitting compare plumbing
  from branch-plan selection at the render sites.
- `build_compare_driven_direct_entry_render_plan()` now owns the shared
  compare-driven direct-target path for plain fallback, compare-join
  continuation, and the short-circuit entry seed plan; future cleanup should
  extend that helper instead of reintroducing case-specific compare-context or
  direct-branch assembly.
- Keep Step 3 lookup cleanup aligned with the shared prepared-control-flow
  helpers in `prealloc.hpp`; prefer those lookups over re-growing local
  `branch_conditions` walks in x86.
- `render_minimal_compare_branch_if_supported()` now depends on prepared
  branch metadata as well; keep future minimal-branch fixes aligned with the
  shared lookup surface instead of re-growing entry-compare inspection.
- `render_loop_join_countdown_if_supported()` and
  `render_materialized_compare_join_if_supported()` now depend on the shared
  prepared-branch lookup surface; if either fast path needs more branch facts,
  strengthen the shared contract rather than reintroducing emitter-local scans.
- `classify_short_circuit_join_incoming()` still assumes the prepared select
  join carries exactly one bool-like immediate lane and one named RHS lane; if
  that invariant changes, repair the shared contract instead of extending
  emitter-local pattern matching.
- Treat any future fix here as capability repair, not expectation weakening:
  the joined branch route is covered by `backend_x86_handoff_boundary`.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The build and narrow proof passed for this Step 3 minimal compare-branch
prepared-control-flow ownership packet; proof output is in `test_after.log`.
