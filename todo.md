Status: Active
Source Idea Path: ideas/open/262_phase_f3_x86_compare_join_loadlocal_selected_arm_support.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Contract Inventory

# Current Packet

## Just Finished

Completed plan Step 1, `Contract Inventory`.

The prepared materialized compare-join path currently builds return-arm
contracts through
`find_prepared_materialized_compare_join_return_context(...)`,
`classify_computed_value(...)`,
`classify_prepared_materialized_compare_join_return_shape(...)`,
`build_prepared_materialized_compare_join_return_arm(...)`, and
`find_prepared_resolved_materialized_compare_join_render_contract(...)` in
`src/backend/prealloc/control_flow.hpp`. The classifier only recognizes
selected roots as immediate, param, same-module `LoadGlobal`, or
pointer-backed same-module `LoadGlobal`; the join-block pre-carrier scan only
indexes `BinaryInst` and `LoadGlobalInst`, so a same-predecessor-block
`LoadLocal` selected arm is rejected before the x86 renderer can consume it.

The x86 render site is
`append_prepared_i32_param_zero_compare_join_return_function(...)` in
`src/backend/mir/x86/module/module.cpp`, which obtains
`find_prepared_param_zero_materialized_compare_join_branches(...)`, resolves a
render contract, rejects unsupported arms through
`prepared_i32_compare_join_return_arm_is_supported(...)`, then renders each arm
with `append_prepared_i32_compare_join_return_arm(...)`. That renderer only
switches on the existing `PreparedComputedBaseKind` variants and therefore has
no path to call
`render_agreed_route3_load_local_statement_memory_operand(...)` for a selected
`LoadLocal`.

Existing selected-arm shapes map as follows:
- immediate and immediate operation chains: `classify_computed_value(...)`
  returns `PreparedComputedBaseKind::ImmediateI32`.
- parameter and parameter operation chains: the same helper returns
  `PreparedComputedBaseKind::ParamValue`.
- same-module globals and global operation chains: the same helper records
  `PreparedComputedBaseKind::GlobalI32Load`; resolution goes through
  `resolve_prepared_materialized_compare_join_return_arm(...)`.
- pointer-backed same-module globals: the same helper records
  `PreparedComputedBaseKind::PointerBackedGlobalI32Load` and resolves both the
  pointer-root global and loaded global.
- trailing-immediate shapes are layered by
  `classify_prepared_materialized_compare_join_return_shape(...)` and
  `find_prepared_materialized_compare_join_return_binary_plan(...)` from
  `PreparedMaterializedCompareJoinReturnContext::trailing_binary`.

The authority facts needed by
`render_agreed_route3_load_local_statement_memory_operand(...)` are: the real
predecessor `Block` and prepared `BlockLabelId`, the exact `LoadLocalInst`
instruction index in that predecessor, the selected result `ValueNameId`, a
Route 3 `Route3MemoryAccessNodeKind::LoadLocal` record whose result matches
the load, and an available `PreparedEdgePublication` whose source producer is
`LoadLocal`, whose source producer block/instruction match the same load, and
whose `source_memory_access` is available frame-slot prepared addressing that
matches the Route 3 local-slot memory facts. Publication facts are populated
through `make_prepared_edge_publication_lookups(...)`,
`apply_source_producer_fact(...)`, and `apply_source_memory_access_fact(...)`
from the real edge-transfer `incoming_value`.

## Suggested Next

Execute Step 2 by adding a narrow prepared contract shape for one selected arm
rooted in a same-predecessor-block `LoadLocal` with prepared local-slot
addressing. The minimal target is to extend the prepared materialized
compare-join return contract so it can carry the predecessor block label,
instruction index, selected result name, and source-memory/publication
authority required by the existing x86 agreement facade. Keep the support
limited to `PreparedJoinTransferKind::EdgeStoreSlot` compare-join selected
arms where the edge-transfer `incoming_value` is the same predecessor-block
`LoadLocal` result.

## Watchouts

- Keep this bounded to same-predecessor-block `LoadLocal` selected arms with
  prepared local-slot addressing.
- Do not revive synthetic `join_transfers` or hand-built publication rows.
- Do not broaden into generic compare-join selected-expression lowering.
- Preserve existing immediate, param, global, pointer-backed global, and
  trailing-immediate compare-join shapes.
- Do not classify a join-block carrier `LoadLocal` as the target shape; the
  needed source producer must be the edge-transfer incoming value rooted in the
  authoritative predecessor block.
- Step 2 should add prepared-side tests around the render-contract helper
  before Step 3 wires x86 rendering.

## Proof

Read-only/design packet. No build was required. Ran
`git diff --check -- todo.md`.
