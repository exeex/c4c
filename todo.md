# EASTL Container Bring-Up Todo

Status: Active
Source Idea: ideas/open/47_eastl_container_bringup_plan.md
Source Plan: plan.md

## Active Item

- Step 2: resume the smallest active EASTL parser frontier from
  `tests/cpp/eastl/eastl_tuple_simple.cpp`, not `eastl_vector_simple.cpp`.

## Completed

- Reopened `ideas/open/47_eastl_container_bringup_plan.md` as the active idea.
- Activated `plan.md` from `ideas/open/47_eastl_container_bringup_plan.md`.
- Preserved the current EASTL baseline from
  `tests/cpp/eastl/README.md` in the active runbook.
- Recorded that Step 1 and Step 2 of the original bring-up ladder are already
  complete enough to resume from Step 3.
- Reproduced the current Step 3 parser blocker from
  `tests/cpp/eastl/eastl_tuple_simple.cpp` and confirmed it still reaches
  `/usr/include/c++/14/bits/ranges_util.h:740`.
- Reduced one generic declaration-vs-expression split bug into focused internal
  parser regressions covering shadowed tag, typedef, and using-alias names.
- Fixed block-scope statement dispatch so visible value bindings followed by
  assignment operators stay on the expression path.

## Next Slice

- rerun `tests/cpp/eastl/eastl_tuple_simple.cpp` and
  `tests/cpp/eastl/eastl_memory_simple.cpp` against the new parser fix
- confirm whether the `ranges_util.h:740` blocker moves forward or exposes the
  next distinct generic parser gap
- if the EASTL frontier still fails, reduce the next blocker from the updated
  trace before touching `eastl_vector_simple.cpp`

## Blockers

- `eastl_tuple_simple.cpp` and `eastl_memory_simple.cpp` have not yet been
  rerun after the shadowed-value assignment parser fix, so the current EASTL
  blocker may have moved
- `eastl_vector_simple.cpp` also times out deeper in the stack, so it is not
  currently the smallest useful frontier

## Resume Notes

- use [tests/cpp/eastl/README.md](/workspaces/c4c/tests/cpp/eastl/README.md) as
  the current stage inventory
- Step 1 foundation cases already parse and mostly fail later in sema
- `eastl_utility_simple.cpp` parse-only now succeeds and is ready for
  canonical/sema follow-up if it becomes the smaller frontier
- focused parser coverage now exists for shadowed-name assignment dispatch
  under `tests/cpp/internal/postive_case/local_value_shadows_*`
- runtime and ABI glue remain explicitly out of scope except for temporary local
  shims already allowed by the source idea
