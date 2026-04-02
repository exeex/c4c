# EASTL Container Bring-Up Todo

Status: Active
Source Idea: ideas/open/eastl_container_bringup_plan.md
Source Plan: plan.md

## Active Item

- Step 2: Foundation headers and traits
- Current slice: compare the newly exposed
  `eastl::has_unique_object_representations` incomplete-type failure against
  Clang and reduce it to the smallest generic semantic testcase before the next
  implementation change

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
- [x] Reduced the shared `type_properties.h` parser blocker to an internal
  `typedef __underlying_type(T) type;` record-member testcase and taught
  `parse_base_type()` to treat `__underlying_type(...)` as a type-producing
  builtin in declaration contexts
- [x] Reclassified the affected EASTL recipes after the parser fix; the Step 2
  and Step 3 fronts now stop on `eastl::has_unique_object_representations`
  becoming incomplete during `--parse-only`

## Next Slice

- compare the `__has_unique_object_representations` spelling and resulting
  record definition against Clang on a reduced internal testcase
- decide whether the new blocker belongs to parse-only semantic validation or a
  more general builtin-type-trait completeness bug before implementing the next
  Step 2 fix

## Blockers

- `eastl_integer_sequence_simple.cpp`, `eastl_type_traits_simple.cpp`,
  `eastl_utility_simple.cpp`, `eastl_memory_simple.cpp`,
  `eastl_tuple_simple.cpp`, and `eastl_vector_simple.cpp` now all stop with
  `object has incomplete type: eastl::has_unique_object_representations` after
  the shared `__underlying_type(T)` parser blocker was removed

## Resume Notes

- activation completed with no prior active `plan.md` or `plan_todo.md`
- the open idea remains the authoritative scope source
- Step 1 inventory is now captured in `tests/cpp/eastl/README.md`
- the foundation batch now has executable parse recipes in
  `tests/cpp/internal/InternalTests.cmake`
- `tests/cpp/internal/parse_only_case/record_member_underlying_type_parse.cpp`
  is the reduced internal regression for the fixed parser blocker
- `eastl_piecewise_construct_simple.cpp` and
  `eastl_tuple_fwd_decls_simple.cpp` parse successfully but first fail during
  canonical/sema expansion with undeclared identifiers from EASTL internals
