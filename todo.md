# Template Arg Transport De-Stringify Todo

Status: Active
Source Idea: ideas/open/51_template_arg_transport_de_stringify_refactor.md
Source Plan: plan.md

## Active Item

- Step 4/5 follow-through after the field deletion slice: migrate remaining
  template-arg transport away from debug-text-heavy storage into fully typed
  per-arg payloads so deferred NTTP and HIR paths can stop reparsing internal
  spellings in the common case.
- Iteration focus: keep the tree green after deleting `tpl_struct_arg_refs`,
  then tighten the new `TemplateArgKind` transport so each arg is stored as an
  actual typed/value payload instead of mostly as `debug_text`.
- Hard acceptance rule remains: this plan is only fully done when the old field
  is gone and the new architecture no longer relies on string-prefix semantic
  recovery.
- Next intended slice: replace the current parallel-array fallback pattern with
  a real per-arg transport object, then migrate parser/HIR rebuild helpers to
  consume `kind/type/value` directly before touching `parser_types_template.cpp`.

## Completed

- Activated `ideas/open/51_template_arg_transport_de_stringify_refactor.md`
  into `plan.md`.
- Rewrote the idea into an execution-oriented runbook with an explicit hard
  acceptance criterion: `tpl_struct_arg_refs` must be completely deleted for the
  refactor to count as done.
- Added `TemplateArgKind` plus structured template-arg storage fields onto
  `TypeSpec` in [ast.hpp](/workspaces/c4c/src/frontend/parser/ast.hpp).
- Deleted `TypeSpec::tpl_struct_arg_refs` and migrated all frontend
  reader/writer sites under `src/frontend/` to the new transport helpers.
- Updated parser/HIR pending-type identity and template rebuild helpers to use
  `encode_template_arg_debug_list(...)` instead of the removed field.
- Verified the field removal mechanically:
  `rg -n "tpl_struct_arg_refs" src/frontend` returns no matches.
- Rebuilt `c4cll` successfully after the migration.
- Re-ran focused regressions successfully:
  `cpp_positive_sema_template_builtin_is_enum_inherited_value_runtime_cpp`
  `cpp_positive_sema_template_builtin_is_enum_qualified_inherited_value_runtime_cpp`
  `cpp_positive_sema_eastl_inherited_trait_value_template_arg_parse_cpp`
  `cpp_eastl_type_traits_parse_recipe`
