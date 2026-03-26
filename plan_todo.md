# Templated Member Scope — Execution State

## Completed
- [x] Step 1: Freeze regression surface — all acceptance targets registered and green
- [x] Step 2: Add explicit template-scope stack type (`TemplateScopeFrame`, `TemplateScopeParam`, `TemplateScopeKind`)
- [x] Step 3: Add push/pop helpers (`push_template_scope`, `pop_template_scope`)
- [x] Step 4: Push enclosing template scope in `declarations.cpp` around `parse_top_level()`
- [x] Step 5: Push member-template scope in `parse_struct_or_union` via `TemplateParamGuard`
- [x] Step 6: Teach type lookup to consult the scope stack (`is_template_scope_type_param()`)
- [x] Step 7: Unify ad hoc `active_template_member_type_params_` with the stack — REMOVED the ad hoc set entirely
- [x] Step 8: Out-of-class member definitions with owner-aware scope
  - Added `owner_struct_tag` to `TemplateScopeFrame`
  - Three paths now enter owner scope: general qualified declarator, out-of-class operator, out-of-class constructor
  - `current_struct_tag_` set to owner during parameter/body parsing of out-of-class member defs
  - Template scope relabeled from `FreeFunctionTemplate` to `EnclosingClass` when owner is a known template struct
  - New test: `out_of_class_member_owner_scope_parse.cpp`
- [x] Step 9: Lock composite nested-scope case — `templated_member_nested_scope_parse` passes
- [x] Step 10: Revalidate template bring-up reductions — all 7 acceptance target tests green
- [x] Step 11: Reprobe eastl_slice7d — passes (already fixed in prior bug fix)
- [x] Step 12: Reprobe std_vector_simple — still fails at `#include <vector>` (std lib header parsing frontier)
  - Blocker: standard library headers require features beyond current parser (complex preprocessor macros, deep template nesting)
  - This is a known frontier limitation, not a regression from the scope work

## Bug fix (prerequisite)
- [x] Fix grouped-declarator false positive for function-type args like `R(Args...)` / `R(A, A)` in `parse_declarator`
  - Root cause: `is_grouped` only checked first token after `(` was identifier; didn't check what followed
  - Fix: reject grouped-declarator when identifier is followed by `,` or `...`
  - Result: `eastl_slice7d_qualified_declarator_parse` now passes (2142/2142, was 2141/2142)

## Plan Status: COMPLETE
All 12 steps are done. The template-scope stack model is in place:
1. Parser has an explicit template-scope stack ✓
2. Templated struct/class bodies push inherited enclosing-class scope ✓
3. Member templates push nested scope on top of inherited class scope ✓
4. Type/declarator lookup consults the scope stack ✓
5. Out-of-class member definitions preserve owner/template association ✓
6. All reduced tests stay green ✓
7. `eastl_slice7d` is fixed ✓
8. `std_vector_simple` frontier re-measured (blocked on std lib headers) ✓

## Notes
- `is_template_scope_type_param()` walks the stack from innermost to outermost, checking non-NTTP params
- All 4 lookup sites (is_type_start, parse_base_type identifier disambiguation, dependent template-template param, template arg type/NTTP disambiguation) now use the scope stack exclusively
- `active_template_member_type_params_` has been completely removed
- `TemplateParamGuard` no longer references it; cleanup is typedef-only + scope stack pop
- Full suite: 2143/2143 (all passing)
