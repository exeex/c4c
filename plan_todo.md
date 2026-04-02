# EASTL Container Bring-Up Todo

Status: Active
Source Idea: ideas/open/eastl_container_bringup_plan.md
Source Plan: plan.md

## Active Item

- Step 2: Foundation headers and traits
- Current slice: isolate the shared `type_properties.h` parser blocker exposed
  by `eastl_integer_sequence_simple.cpp`, `eastl_type_traits_simple.cpp`, and
  `eastl_utility_simple.cpp`, then reduce it to the smallest generic parser
  testcase before implementation work

## Todo

- [x] Step 1: Inventory and recipe normalization
- [ ] Step 2: Foundation headers and traits
- [ ] Step 3: Object lifetime and tuple layer
- [ ] Step 4: `eastl::vector` parse frontier
- [ ] Step 5: `eastl::vector` semantic, HIR, and lowering viability
- [ ] Step 6: Follow-on container expansion

## Completed

- [x] Activated `ideas/open/eastl_container_bringup_plan.md` into the active
  runbook
- [x] Enumerated all eight `tests/cpp/eastl/*` cases and recorded their shared
  include flags, bounded commands, and earliest failing stage in
  `tests/cpp/eastl/README.md`
- [x] Added explicit parse recipe coverage for the five foundation cases so the
  current inventory is executable through CTest

## Next Slice

- reproduce the shared `type_properties.h` parser failure with the smallest
  internal testcase possible
- compare the reduced parser failure against Clang acceptance before attempting
  a frontend fix for the Step 2 foundation batch

## Blockers

- `eastl_integer_sequence_simple.cpp`, `eastl_type_traits_simple.cpp`,
  `eastl_utility_simple.cpp`, `eastl_memory_simple.cpp`,
  `eastl_tuple_simple.cpp`, and `eastl_vector_simple.cpp` all currently stop in
  parser handling of `ref/EASTL/include/EASTL/internal/type_properties.h:35:52`
  with `parse_fn=try_parse_record_typedef_member phase=committed expected=SEMI
  got='('`

## Resume Notes

- activation completed with no prior active `plan.md` or `plan_todo.md`
- the open idea remains the authoritative scope source
- Step 1 inventory is now captured in `tests/cpp/eastl/README.md`
- the foundation batch now has executable parse recipes in
  `tests/cpp/internal/InternalTests.cmake`
- `eastl_piecewise_construct_simple.cpp` and
  `eastl_tuple_fwd_decls_simple.cpp` parse successfully but first fail during
  canonical/sema expansion with undeclared identifiers from EASTL internals
