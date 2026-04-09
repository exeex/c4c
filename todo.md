# Rvalue Reference Completeness Todo

Status: Active
Source Idea: ideas/open/46_rvalue_reference_completeness_plan.md
Source Plan: plan.md

## Active Item

- Step 1: audit existing `&&` coverage and classify the current regression
  surface by compiler stage.

## Completed

- Activated `plan.md` from `ideas/open/46_rvalue_reference_completeness_plan.md`.

## Next Slice

- inventory existing parser, sema, HIR, and runtime `&&` tests
- choose the narrowest missing shape that blocks the next test-first slice
- record any adjacent work as bounded follow-on instead of mutating the plan

## Blockers

- none recorded

## Resume Notes

- start from current in-tree `&&` coverage rather than guessing missing cases
- prefer parser blockers first when they stop library headers from parsing
- keep EASTL or libstdc++ findings scoped to generic language defects only
