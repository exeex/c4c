# Active Todo

Status: Active
Source Idea: ideas/open/43_c_style_cast_reference_followups_review.md
Source Plan: plan.md

## Active Item

- Step 1: audit reference-qualified cast targets
- Current slice: inventory existing C-style cast reference coverage and add the
  first missing focused reductions for `&` / `&&` target forms before touching
  parser or sema behavior
- Current implementation target: `tests/cpp` cast/reference regressions plus
  the earliest failing parser/sema surface those tests expose
- Next intended slice: classify the first unresolved reference-cast mismatch by
  earliest failing stage and fix one root cause at a time with Clang comparison
  for value-category-sensitive behavior

## Completed

- Activated `ideas/open/43_c_style_cast_reference_followups_review.md` into the
  active runbook after closing idea 42
