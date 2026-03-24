# Lazy Template Instantiation — Execution State

## Active Item
**Step 5: Tighten Engine Progress Semantics Only If Needed**

## Completed
- **Step 1: Expand Use-Site Seeding** — added `seed_pending_template_type(...)` at all 5 previously unseeded `resolve_pending_tpl_struct_if_needed(...)` call sites:
  - L2185: `resolve_struct_member_typedef_hir` → `apply_bindings` lambda (MemberTypedef)
  - L2727: nested template struct in template args (OwnerStruct)
  - L2923: base type during struct instantiation (BaseType)
  - L7055: direct NameRef template instantiation in `lower_expr` (OwnerStruct)
  - L7113: scope-qualified static member NameRef in `lower_expr` (OwnerStruct)
  - All existing tests pass (2122/2123, pre-existing failure unchanged)

- **Step 2: Make Blocked Paths Spawn Explicit Owner Work** — three improvements:
  1. OwnerStruct case: detect missing primary template def → return `terminal` instead of infinite `blocked`
  2. Non-OwnerStruct case (DeclarationType, CastTarget, BaseType): detect missing primary early → return `terminal` before spawning doomed owner work
  3. MemberTypedef case: detect missing primary on owner → return `terminal` before spawning doomed owner work
  4. Engine: terminal items now marked as resolved (removed from retry loop) while still recording diagnostic — prevents spin without progress
  - All existing tests pass (2122/2123, pre-existing failure unchanged)

- **Step 3: Split `resolve_pending_tpl_struct(...)` Into Small Helpers** — four extracted helpers:
  1. `eval_deferred_nttp_expr_hir(...)` — lambda promoted to private method; evaluates deferred NTTP default expressions with recursive descent parser
  2. `materialize_template_args(...)` — resolves explicit and default template args into `ResolvedTemplateArgs` (concrete_args + type/nttp bindings)
  3. `build_template_mangled_name(...)` — static helper that builds the mangled/concrete struct name from primary, selected pattern, and concrete args
  4. `instantiate_template_struct_body(...)` — creates `HirStructDef` with fields, bases, methods; registers in module
  - `resolve_pending_tpl_struct(...)` is now a thin 7-step coordinator
  - Added `ResolvedTemplateArgs` result struct
  - All existing tests pass (2122/2123, pre-existing failure unchanged)

- **Step 4: Move Decision-Making Out Of Recursive Helpers** — two changes:
  1. Coordinator: after `materialize_template_args`, check if any type arg still has `tpl_struct_origin` set (unresolved nested template); if so, bail out early leaving `ts.tpl_struct_origin` intact — the engine will retry after the prerequisite work resolves the nested type
  2. `instantiate_template_struct_body`: removed inline `resolve_pending_tpl_struct_if_needed` for base types; base type prerequisite work is seeded only, engine owns the retry
  - Retry behavior is now visible in the engine worklist model, not hidden in helper recursion
  - All existing tests pass (2122/2123, pre-existing failure unchanged)

## Current Slice
Step 5: Tighten Engine Progress Semantics Only If Needed — only touch the engine loop if the current blocked/spawned-new-work accounting is insufficient.

## Next Intended Slice
Plan complete after step 5 (assess if any engine changes needed).

## Blockers
None
