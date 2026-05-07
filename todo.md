# Current Packet

Status: Active
Source Idea Path: ideas/open/148_hir_static_member_carrier_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish The Blocked HIR Static-Member Baseline

## Just Finished

Step 1 inventory completed for the HIR static-member carrier authority
decomposition baseline. The preserved rejected-route log `test_after.log`
confirms the five known failing tests:
`cpp_positive_sema_template_variable_alias_inherited_member_typedef_runtime_cpp`,
`cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp`,
`cpp_positive_sema_template_variable_member_typedef_normalization_runtime_cpp`,
`cpp_positive_sema_template_variable_trait_runtime_cpp`, and
`cpp_hir_template_inherited_member_typedef_trait`.

Visible carrier seams were inventoried without editing implementation files.
In `src/frontend/hir/hir_types.cpp`, static-member carrier state is split
between rendered fallback maps (`struct_static_member_decls_`,
`struct_static_member_const_values_`) and structured owner/member maps
(`struct_static_member_decls_by_owner_`,
`struct_static_member_const_values_by_owner_`). `lower_struct_def()` registers
static members by rendered `tag`/`f->name` and, when a complete
`HirRecordOwnerKey` exists, also registers owner-keyed lookups. The
owner-keyed static-member const-value reader still falls back through rendered
tag/member spelling when the structured value misses.

In `src/frontend/hir/impl/expr/scalar_control.cpp`, scoped static-member
lowering starts from rendered `n->name` split at `::`, derives
`structured_member` from `unqualified_name`/`unqualified_text_id`, then tries
to recover owner authority from a synthetic owner `TypeSpec`. That makes the
first unresolved boundary the split between member identity carrier and owner
authority carrier, not the static-member value tables themselves.

## Suggested Next

Next packet: Step 2 focused probe target should isolate
`lower_var_expr()` scoped static-member lowering in
`src/frontend/hir/impl/expr/scalar_control.cpp`: prove whether
`structured_member_text_id` is correct for the failing `::value` references
while `record_owner_key`/`owner_tag` authority is missing or rendered-derived.

## Watchouts

- The failed proof log is preserved in `test_after.log`; do not overwrite it
  unless the supervisor delegates new proof output.
- The blocked idea 147 remains open. This plan exists only to define the HIR
  static-member carrier policy needed before returning to idea 147 Step 4.
- Rendered `expr->name` must not provide owner authority. Member identity may
  use `unqualified_text_id`, `unqualified_name`, or generated payload metadata
  only after structured owner authority exists.
- `hir_types.cpp` has compatibility fallback readers for static-member decls
  and const values; Step 2 should inspect before changing so the probe does not
  collapse member identity and owner authority into another rendered bridge.

## Proof

Inventory/status packet only. No code proof required and no implementation or
test files were edited. `test_after.log` was preserved as the rejected-route
diagnosis log. Required proof for this packet: `git diff --check -- todo.md`.
