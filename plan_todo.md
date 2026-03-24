# Lazy Template Instantiation — Execution State

## Active Item
**Step 2: Make Blocked Paths Spawn Explicit Owner Work**

## Completed
- **Step 1: Expand Use-Site Seeding** — added `seed_pending_template_type(...)` at all 5 previously unseeded `resolve_pending_tpl_struct_if_needed(...)` call sites:
  - L2185: `resolve_struct_member_typedef_hir` → `apply_bindings` lambda (MemberTypedef)
  - L2727: nested template struct in template args (OwnerStruct)
  - L2923: base type during struct instantiation (BaseType)
  - L7055: direct NameRef template instantiation in `lower_expr` (OwnerStruct)
  - L7113: scope-qualified static member NameRef in `lower_expr` (OwnerStruct)
  - All existing tests pass (2122/2123, pre-existing failure unchanged)

## Current Slice
Inspect `instantiate_deferred_template_type(...)` cases for MemberTypedef, BaseType, DeclarationType, CastTarget — ensure blocked owner-dependent work spawns OwnerStruct prerequisite and returns `blocked` instead of falling through to `terminal`.

## Next Intended Slice
Step 3: Split `resolve_pending_tpl_struct(...)` into small helpers

## Blockers
None
