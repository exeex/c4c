# Parser Whitelist Boundary Audit Todo

Status: Active
Source Idea: ideas/open/07_parser_whitelist_boundary_audit.md
Source Plan: plan.md
Last Updated: 2026-03-29

## Active Item

- Step 1: inventory parser sites that are still not whitelist-led, starting
  with `requires` skipping, generic record-member recovery, broad start probes,
  and `NK_EMPTY` discard paths

## Completed

- switched the active runbook away from the `std::vector` bring-up and folded
  the latest parser-hygiene findings back into
  `ideas/open/04_std_vector_bringup_plan.md`
- activated `ideas/open/07_parser_whitelist_boundary_audit.md` as the new
  active plan
- captured the most recent reduced evidence motivating the switch:
  - `iterator_concepts.h` plus `functional_hash.h` had reduced to a
    `requires`-clause boundary issue
  - tightening `skip_cpp20_constraint_atom(...)` improved the reduced repro
  - added
    `tests/cpp/internal/postive_case/cpp20_requires_trait_disjunction_function_parse.cpp`
    to hold that narrower case in-tree

## Next Slice

- build a curated inventory with at least these buckets:
  - constraint / `requires` skipping helpers
  - generic token-skipping recovery helpers
  - broad declaration / type-start probes
  - `NK_EMPTY` parse-and-discard branches
- tag each site as:
  - acceptable breadth
  - suspicious breadth
  - known bug / already implicated in a reduced repro
- prioritize the first batch by direct evidence from:
  - `iterator_concepts.h` / `functional_hash.h`
  - record-member recovery swallowing to `}`
  - top-level constrained function templates being dropped or mis-associated

## Notes

- this switch is intentional: recent debugging showed that continuing `<vector>`
  reduction on top of broad parser rules was becoming lower-leverage than
  tightening the parser boundaries themselves
- the parked `std::vector` idea should be revisited after the first whitelist
  audit fixes land and the remaining suspicious sites are ranked
- current uncommitted implementation work in the tree still includes:
  - tightened `requires`-clause boundary logic in
    `src/frontend/parser/declarations.cpp`
  - the new reduced regression test for requires-trait disjunction parsing

## Blockers

- none yet
