# Templated Member Scope — Execution State

## Completed
- [x] Step 1: Freeze regression surface — all acceptance targets registered and green
- [x] Step 2: Add explicit template-scope stack type (`TemplateScopeFrame`, `TemplateScopeParam`, `TemplateScopeKind`)
- [x] Step 3: Add push/pop helpers (`push_template_scope`, `pop_template_scope`)
- [x] Step 4: Push enclosing template scope in `declarations.cpp` around `parse_top_level()`
- [x] Step 5: Push member-template scope in `parse_struct_or_union` via `TemplateParamGuard`

## Bug fix (prerequisite)
- [x] Fix grouped-declarator false positive for function-type args like `R(Args...)` / `R(A, A)` in `parse_declarator`
  - Root cause: `is_grouped` only checked first token after `(` was identifier; didn't check what followed
  - Fix: reject grouped-declarator when identifier is followed by `,` or `...`
  - Result: `eastl_slice7d_qualified_declarator_parse` now passes (2142/2142, was 2141/2142)

## In Progress
- [ ] Step 6: Teach type lookup to consult the scope stack
- [ ] Step 7: Unify ad hoc `active_template_member_type_params_` with the stack

## Remaining
- [ ] Step 8: Out-of-class member definitions with owner-aware scope
- [ ] Step 9: Lock composite nested-scope case
- [ ] Step 10: Revalidate template bring-up reductions
- [ ] Step 11: Reprobe eastl_slice7d
- [ ] Step 12: Reprobe std_vector_simple

## Notes
- Template scope stack is wired into declarations.cpp (top-level template) and types.cpp (member template in struct body)
- `TemplateParamGuard` destructor now pops the member-template scope automatically
- Existing `active_template_member_type_params_` still in use as the actual lookup mechanism; stack is metadata-only for now
- Full suite: 2142/2142 (all passing)
