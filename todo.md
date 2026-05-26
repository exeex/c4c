Status: Active
Source Idea Path: ideas/open/37_aarch64_branch_and_entry_foldback_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Fold Comparison Branch Fusion Into Comparison Owner

# Current Packet

## Just Finished

Step 2 - Fold Comparison Branch Fusion Into Comparison Owner completed the
mechanical branch-fusion ownership packet:

- Moved the public branch-fusion declarations formerly in
  `comparison_branch_fusion.hpp` into `comparison.hpp`, including
  `DispatchBranchFusionHooks`, `branch_condition_suffix`,
  `is_cmp_immediate_encodable`, and the fused compare/conditional-branch
  lowering helpers still used by dispatch/materialization/calls owners.
- Moved the implementation formerly in `comparison_branch_fusion.cpp` into
  `comparison.cpp`.
- Replaced live `comparison_branch_fusion.hpp` include sites with
  `comparison.hpp`.
- Removed `comparison_branch_fusion.cpp` from `src/backend/CMakeLists.txt`.
- Deleted obsolete `comparison_branch_fusion.cpp` and
  `comparison_branch_fusion.hpp`.
- Kept prepared branch-condition use, condition-code mapping, branch target
  selection, diagnostics, and test expectations unchanged.

## Suggested Next

Next mechanical fold-back packet: fold `prologue_entry_formals.cpp` into the
prologue owner. Preserve the existing `lower_entry_formal_publications`
prologue API for `dispatch.cpp`, move implementation-private entry-formal
helpers into `prologue.cpp`, delete the obsolete helper translation unit, and
remove it from `src/backend/CMakeLists.txt`.

## Watchouts

Keep this route mechanical. Do not change prepared branch-condition authority,
condition-code policy, branch target selection, ABI entry formal layout, byval
entry-copy semantics, F128 carrier handling, diagnostics, or tests. The next
packet should stay on prologue entry-formal ownership only and should not
rework dispatch routing or formal-publication planning.

## Proof

Passed:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_branch_compare_records|backend_aarch64_compare_branch_candidate_records|backend_aarch64_branch_compare_contract|backend_aarch64_prepared_branch_records|backend_aarch64_branch_control_lowering|backend_aarch64_instruction_dispatch)$' | tee test_after.log`

`test_after.log` is the proof log.
