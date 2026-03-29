# std::vector Bring-up Todo

Status: Active
Source Idea: ideas/open/std_vector_bringup_plan.md
Source Plan: plan.md

## Active Item

- Step 1 / Step 2: reproduce the current `std_vector_simple.cpp` parse failure,
  localize the unmatched-brace / parse-state-loss region, and add the smallest
  reduced failing test for this exact parser blocker

## Completed

- activated the open `std::vector` bring-up idea into `plan.md`

## Next Intended Slice

- inspect the failing header region, determine the construct that leaves parser
  state unbalanced, and land a reduced parser regression test plus the smallest
  fix for it

## Blockers

- none recorded yet

## Resume Notes

- `plan.md` did not exist at iteration start; the source idea was activated from
  `ideas/open/std_vector_bringup_plan.md`
- current documented repro command: `./build/c4cll --parse-only tests/cpp/std/std_vector_simple.cpp`
- compare-mode validation is deferred until the case reaches codegen
