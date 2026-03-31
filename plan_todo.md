# Unified Parser State Snapshot/Restore — Execution State

Source Idea: ideas/open/18_parser_state_snapshot_restore.md
Source Plan: plan.md
Status: Complete

## Done

- [x] Step 1: Audit parser state fields and manual save/restore sites
- [x] Step 2: Implement ParserSnapshot, save_state(), restore_state()
- [x] Step 3: Implement TentativeParseGuard
- [x] Step 4: Replace manual save/restore sites with TentativeParseGuard
- [ ] Step 5: Add debug assertion for new fields (optional — skipped; not needed for correctness)
- [x] Step 6: Full regression validation — 2534/2671 tests pass (baseline maintained)

## Sites Converted

| File | Function | Fields saved | Converted? |
|------|----------|-------------|-----------|
| statements.cpp | parse_if_condition_decl | pos_, var_types_ | yes |
| statements.cpp | for-loop range-for detection | pos_, typedefs_, user_typedefs_, typedef_types_ | yes |
| types_declarator.cpp | try_parse_template_type_arg | pos_ | yes |
| types_declarator.cpp | try_parse_template_non_type_expr | pos_ | yes |
| types_declarator.cpp | try_parse_template_non_type_arg (Identifier) | pos_ | yes |
| types_declarator.cpp | consume_qualified_type_spelling_with_typename | pos_ | yes |
| types_declarator.cpp | consume_qualified_type_spelling | pos_ | yes |
| types_declarator.cpp | consume_template_parameter_type_head | pos_ | yes |
| types_declarator.cpp | consume_template_args_followed_by_scope | pos_ | yes (removed redundant save) |
| types_declarator.cpp | consume_member_pointer_owner_prefix | pos_ | yes |
| types_declarator.cpp | try_parse_declarator_member_pointer_prefix | pos_ | yes (simplified) |
| types_declarator.cpp | is_parenthesized_pointer_declarator_start (probe) | pos_ | yes |
| types_declarator.cpp | parse_normal_declarator_tail | pos_ | yes |
| types_struct.cpp | template param default type parse | pos_ | yes |
| types_struct.cpp | typed NTTP parse (+ nested default) | pos_ | yes |
| types_struct.cpp | field peek for static/constexpr | pos_ | yes |
| types_struct.cpp | parse_record_prebody_setup specialization | pos_ | yes |
| types_base.cpp | alias template argument parse | pos_ | yes |
| expressions.cpp | sizeof disambiguation | pos_ | yes |
| expressions.cpp | parse_operator_ref | pos_ | yes |
| expressions.cpp | builtin type trait parse | pos_ | yes |
| expressions.cpp | lambda parse | pos_ | yes |
| expressions.cpp | template arg list in expr | pos_ | yes |
| expressions.cpp | functional cast T(expr) | pos_ + last_resolved_typedef_ | yes |
| declarations.cpp | template constrained param | pos_ | yes |

## Sites Left Unchanged (Too Complex / Uses tokens_ Swap)

| File | Function | Reason |
|------|----------|--------|
| declarations.cpp | parse_global_decl (saved_special_member_pos) | Captured by multiple inner lambdas; converting requires lambda refactor |
| expressions.cpp | cast disambiguation (saved_tokens) | Also saves tokens_ which TentativeParseGuard does not snapshot |
| expressions.cpp | new-expr placement args (saved) | Not a real tentative parse; just re-consumes '(' |
| types_base.cpp | token injection for base type | Swaps tokens_; intentional injection pattern |
| types_template.cpp | token injection for struct | Swaps tokens_; intentional injection pattern |
