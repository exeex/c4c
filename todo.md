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
- Reduced and fixed the out-of-class constructor-template delegating-init parse
  blocker for dependent `typename _Build_index_tuple<...>::__type()` temporaries.
- Confirmed `tests/cpp/eastl/eastl_tuple_simple.cpp` now parses past the old
  `ranges_util.h:740` and tuple delegating-constructor blockers into the older
  canonical/sema undeclared-identifier cluster.

## Next Slice

- classify the renewed `eastl_tuple_simple.cpp` failures as sema/canonical work
  versus any residual parser fallout
- inspect whether the old undeclared-identifier cluster around `mPart0` /
  `mPart1` is the same shared Step 2 sema frontier already seen in simpler
  EASTL cases
- rerun `eastl_memory_simple.cpp` with a fresh bounded parser trace and decide
  whether its next smallest blocker is still parser-side or has also moved into
  later semantic work

## Blockers

- `eastl_tuple_simple.cpp` no longer stops on the old parser blockers, but it
  now ends in a larger canonical/sema undeclared-identifier cluster including
  `mPart0` / `mPart1`
- `eastl_memory_simple.cpp` still times out under parse pressure, though the
  trace now reaches much later tuple/ranges work than before
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
- focused parser coverage now also exists for out-of-class constructor-template
  delegating init arguments under
  `tests/cpp/internal/postive_case/ctor_init_out_of_class_dependent_typename_index_tuple_parse.cpp`
- runtime and ABI glue remain explicitly out of scope except for temporary local
  shims already allowed by the source idea
