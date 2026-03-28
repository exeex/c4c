# Positive Template/Sema Regression Todo

Status: Active
Source Idea: ideas/open/template_positive_sema_regressions_plan.md
Source Plan: plan.md

## Active Item

- Step 2: add focused regression coverage and fix pack expansion parsing inside
  template argument lists for
  `cpp_positive_sema_eastl_probe_pack_expansion_template_arg_parse_cpp`

## In Progress

- Prepare the next mechanism-focused slice for
  `cpp_positive_sema_eastl_probe_call_result_lvalue_frontend_cpp`

## Completed

- Selected `ideas/open/template_positive_sema_regressions_plan.md` as the
  active source idea
- Created `plan.md` and `plan_todo.md` for execution
- Captured the full-suite baseline in `test_before.log`
- Classified the first patch target as a `PARSE_FAIL` in
  `cpp_positive_sema_eastl_probe_pack_expansion_template_arg_parse_cpp`
- Added focused parser coverage in
  `template_pack_expansion_template_arg_expr_parse.cpp`
- Fixed expression-side template-id parsing to accept trailing `...` after a
  successfully parsed template argument list
- Verified targeted parser cases pass:
  `eastl_probe_pack_expansion_template_arg_parse`,
  `template_pack_expansion_template_arg_expr_parse`,
  `template_template_param_parse`, and `template_variadic_qualified_parse`
- Captured a clean full-suite validation in `test_after.log`
- Verified monotonic full-suite results:
  `test_before.log` = 7 failing / 2236 total,
  `test_after.log` = 6 failing / 2237 total

## Next Slice

- Characterize `cpp_positive_sema_eastl_probe_call_result_lvalue_frontend_cpp`
  and isolate the lvalue/call-result frontend mechanism behind
  `error: StmtEmitter: cannot take lval of expr`

## Blockers

- None recorded

## Resume Notes

- Start from the named six positive template/sema tests in `plan.md`
- First slice is the parser mechanism for `Types...` inside nested template
  argument lists such as `__and_<is_default_constructible<Types>..., ...>`
- The pack-expansion parse failure is fixed; remaining named failures are the
  call-result lvalue frontend case, initializer-list runtime case, dependent
  base-expression parse/runtime case, template arg deduction, mixed params, and
  template type substitution
