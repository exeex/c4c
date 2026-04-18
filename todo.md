# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow packet in
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` by collapsing
compare-join continuation and plain conditional compare-driven entry routing
onto one shared direct-target helper contract. Plain fallback now resolves
direct branch targets through `build_plain_cond_direct_branch_targets()`,
compare-join continuation resolves them through
`build_direct_branch_targets_from_continuation()`, and
`build_compare_driven_direct_entry_render_plan()` now owns the common
compare-context plus direct-branch-plan assembly instead of keeping a
continuation-only wrapper.

## Suggested Next

The next small Step 3 packet is to inspect the remaining compare-driven entry
helpers and decide whether the short-circuit entry path can share more of the
common compare-context/render-plan plumbing without weakening prepared
join-transfer ownership checks or widening into idea 59 instruction-selection
scope.

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
  compare-driven direct-target path for plain fallback and compare-join
  continuation; future cleanup should extend that helper instead of
  reintroducing continuation-only branch-plan assembly.
- `classify_short_circuit_join_incoming()` still assumes the prepared select
  join carries exactly one bool-like immediate lane and one named RHS lane; if
  that invariant changes, repair the shared contract instead of extending
  emitter-local pattern matching.
- Treat any future fix here as capability repair, not expectation weakening:
  the joined branch route is covered by `backend_x86_handoff_boundary`.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The build and narrow proof passed for this Step 3 shared compare-driven
direct-target helper cleanup packet; proof output is in `test_after.log`.
