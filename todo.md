Status: Active
Source Idea Path: ideas/open/37_aarch64_branch_and_entry_foldback_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Branch And Entry Helper Ownership

# Current Packet

## Just Finished

Step 1 - Inventory Branch And Entry Helper Ownership completed the
no-implementation inventory packet:

- `comparison_branch_fusion.hpp` exposes `DispatchBranchFusionHooks`,
  `branch_condition_suffix`, `is_cmp_immediate_encodable`, and the fused
  compare/conditional-branch lowering helpers. The implementation file also
  has namespace-local support helpers for integer width, same-block producer
  lookup, prepared frame-slot addressing, assembler instruction creation, and
  selected-operand checks.
- External branch-fusion call sites are in `dispatch.cpp` for the fused branch
  lowering helpers and hook construction, plus `fp_value_materialization.cpp`,
  `dispatch_edge_copies.cpp`, `dispatch_value_materialization.cpp`,
  `cast_ops.cpp`, and `calls.cpp` for the shared condition suffix and cmp
  immediate helpers.
- Live include sites for `comparison_branch_fusion.hpp` are
  `dispatch.cpp`, `fp_value_materialization.cpp`, `dispatch_edge_copies.cpp`,
  `dispatch_value_materialization.cpp`, `cast_ops.cpp`, `calls.cpp`, and
  `comparison_branch_fusion.cpp` itself.
- `prologue_entry_formals.cpp` defines `lower_entry_formal_publications` plus
  namespace-local entry-formal helpers for ABI register source selection,
  stack/register home publication lines, byval aggregate stores, incoming stack
  offset calculation, F128-capable register views, and publication instruction
  construction.
- The only external entry-formal API is
  `lower_entry_formal_publications` declared in `prologue.hpp` and called from
  `dispatch.cpp`. `prologue.hpp` is also included by `prologue.cpp` and
  `traversal.cpp` for existing prologue/frame boundary ownership.
- Build metadata entries to remove during fold-back are
  `mir/aarch64/codegen/comparison_branch_fusion.cpp` and
  `mir/aarch64/codegen/prologue_entry_formals.cpp` in
  `src/backend/CMakeLists.txt`.
- Public declarations that must remain reachable after folding:
  `DispatchBranchFusionHooks`, all currently declared branch-fusion helpers
  needed by dispatch/materialization/calls owners, and
  `lower_entry_formal_publications` for dispatch.

## Suggested Next

Recommended first mechanical fold-back packet: fold
`comparison_branch_fusion.cpp` / `.hpp` into the comparison owner. Move the
declarations still used by external AArch64 owners into `comparison.hpp`, move
the implementation into `comparison.cpp`, replace include sites with
`comparison.hpp`, delete the obsolete branch-fusion helper files, and remove
`comparison_branch_fusion.cpp` from `src/backend/CMakeLists.txt`.

## Watchouts

Keep this route mechanical. Do not change prepared branch-condition authority,
condition-code policy, branch target selection, ABI entry formal layout, byval
entry-copy semantics, F128 carrier handling, diagnostics, or tests. The
branch-fusion helpers are not comparison-private today; several AArch64 owners
still need the shared suffix/immediate helpers and dispatch still needs the
fused branch lowering API after relocation.

## Proof

No build required for inventory-only lifecycle update.

`git diff --check -- todo.md`
