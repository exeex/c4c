# Templated Member Scope — Execution State

## Completed
- [x] Step 1: Freeze regression surface — all acceptance targets registered and green
- [x] Step 2: Add explicit template-scope stack type (`TemplateScopeFrame`, `TemplateScopeParam`, `TemplateScopeKind`)
- [x] Step 3: Add push/pop helpers (`push_template_scope`, `pop_template_scope`)
- [x] Step 4: Push enclosing template scope in `declarations.cpp` around `parse_top_level()`
- [x] Step 5: Push member-template scope in `parse_struct_or_union` via `TemplateParamGuard`
- [x] Step 6: Teach type lookup to consult the scope stack (`is_template_scope_type_param()`)
- [x] Step 7: Unify ad hoc `active_template_member_type_params_` with the stack — REMOVED the ad hoc set entirely

## Bug fix (prerequisite)
- [x] Fix grouped-declarator false positive for function-type args like `R(Args...)` / `R(A, A)` in `parse_declarator`
  - Root cause: `is_grouped` only checked first token after `(` was identifier; didn't check what followed
  - Fix: reject grouped-declarator when identifier is followed by `,` or `...`
  - Result: `eastl_slice7d_qualified_declarator_parse` now passes (2142/2142, was 2141/2142)

## Remaining
- [ ] Step 8: Out-of-class member definitions with owner-aware scope
- [ ] Step 9: Lock composite nested-scope case
- [ ] Step 10: Revalidate template bring-up reductions
- [ ] Step 11: Reprobe eastl_slice7d
- [ ] Step 12: Reprobe std_vector_simple

## Notes
- `is_template_scope_type_param()` walks the stack from innermost to outermost, checking non-NTTP params
- All 4 lookup sites (is_type_start, parse_base_type identifier disambiguation, dependent template-template param, template arg type/NTTP disambiguation) now use the scope stack exclusively
- `active_template_member_type_params_` has been completely removed
- `TemplateParamGuard` no longer references it; cleanup is typedef-only + scope stack pop
- Full suite: 2142/2142 (all passing)
