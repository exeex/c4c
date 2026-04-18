# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow packet in
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` by folding plain
conditional entry's prepared branch-condition validation into
`build_short_circuit_entry_direct_branch_targets()`. The plain-entry
branch-plan path now resolves validated prepared targets through one helper and
hands them to `build_direct_branch_plan_from_targets()` instead of re-checking
the prepared contract inside `build_short_circuit_entry_direct_branch_plan()`.

## Suggested Next

The next small Step 3 packet is to keep tightening compare-driven branch-plan
ownership by giving plain conditional fallback the same dedicated entry helper
shape as the short-circuit path, so `build_compare_driven_entry_render_plan()`
no longer has to wire separate inline lambdas for prepared direct-branch plan
selection versus plain fallback plan selection.

## Watchouts

- Do not activate umbrella idea 57 as executable work.
- Do not pull in idea 59 instruction-selection scope.
- Do not solve coverage gaps with x86 testcase-shaped matcher growth.
- Keep the compare-join lane aligned with the continuation lane: only the
  prepared predecessor recorded by the selected join-transfer edge is
  authoritative for continuation entry.
- `find_short_circuit_join_context()` now owns only prepared select-join
  discovery and transfer-shape validation; keep join-block lookup, prepared
  join-branch lookup, and continuation-plan assembly in
  `build_short_circuit_join_context_from_transfer()` instead of re-growing
  those checks beside join discovery.
- `build_short_circuit_join_context_from_transfer()` now also owns short-
  circuit lane classification; keep join-input ownership validation there
  instead of re-growing `classify_short_circuit_join_incoming()` calls in entry
  routing or render paths.
- `build_direct_branch_plan_from_labels()` now owns direct true/false target
  lookup for short-circuit entry routing and plain cond-branch compare-driven
  fallback; keep branch-target null/self checks there instead of re-growing
  local `resolve_direct_branch_targets()` plumbing in compare render paths.
- `build_compare_join_continuation()` remains the Step 3 gate for the join-
  result zero-compare contract; keep `Eq`/`Ne` mapping and jump-target choice
  data-driven there instead of pushing them back into renderer assembly.
- `resolve_direct_branch_targets()` now owns direct true/false block lookup for
  plain cond-branch entry and compare-join continuation successor selection;
  keep label-to-block resolution there, but keep branch-plan validity checks in
  `build_direct_branch_plan_from_targets()` instead of re-growing local
  pointer/null/self-target checks in emitter paths.
- `build_short_circuit_entry_direct_branch_targets()` now owns the prepared
  branch-condition contract for plain conditional entry; keep condition-name,
  compare-shape, and label-resolution validation there instead of re-growing
  those checks in `build_short_circuit_entry_direct_branch_plan()` or render
  call sites.
- `build_compare_join_branch_plan()` now owns the compare-join continuation
  handoff into `build_direct_branch_plan_from_targets()`; keep future
  continuation target plumbing there instead of re-growing plan assembly or
  local continuation-target checks beside render sites.
- `CompareDrivenBranchRenderPlan` now owns the compare setup/opcode plus
  branch-lane targets that feed `render_compare_driven_branch_plan()`; future
  cleanup should extend that contract instead of re-splitting compare plumbing
  from branch-plan selection at the render sites.
- `build_compare_driven_entry_render_plan()` now owns entry-path assembly of
  `ShortCircuitEntryCompareContext` plus the final
  `CompareDrivenBranchRenderPlan`; keep future entry cleanup inside that helper
  instead of re-growing local compare-context/branch-plan pairing.
- `classify_short_circuit_join_incoming()` still assumes the prepared select
  join carries exactly one bool-like immediate lane and one named RHS lane; if
  that invariant changes, repair the shared contract instead of extending
  emitter-local pattern matching.
- Treat any future fix here as capability repair, not expectation weakening:
  the joined branch route is covered by `backend_x86_handoff_boundary`.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The build and narrow proof passed for this Step 3 direct-branch helper
plain-entry validation cleanup packet; proof output is in `test_after.log`.
