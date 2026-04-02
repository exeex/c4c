# EASTL Container Bring-Up Todo

Status: Active
Source Idea: ideas/open/eastl_container_bringup_plan.md
Source Plan: plan.md

## Active Item

- Step 2: Foundation headers and traits
- Current slice: reduce the remaining `eastl::is_signed_helper` blocker to the
  interaction between defaulted NTTPs and alias-template base expressions after
  the inherited aliased-base `::type` lookup fix

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
- [x] Reduced the fallback `integral_constant<bool, is_integral_v<...>>`
  record-base failure to
  `tests/cpp/internal/parse_only_case/record_base_variable_template_value_arg_parse.cpp`
  and taught template-argument expression parsing to stop on enclosing
  template-close tokens instead of consuming them as shift / relational
  operators
- [x] Reclassified the affected EASTL parse recipes after the parser fix:
  `eastl_integer_sequence_simple.cpp` and `eastl_type_traits_simple.cpp` now
  stop on `eastl::is_signed_helper`, `eastl_utility_simple.cpp` now passes
  parse-only, and `eastl_vector_simple.cpp` advances to a new parser error in
  `EASTL/internal/function_detail.h`
- [x] Reduced one generic `is_signed_helper` sub-case to
  `tests/cpp/internal/postive_case/inherited_type_alias_base_member_lookup_parse.cpp`
  and taught recursive member-typedef lookup to follow aliased bases such as
  `false_type` instead of only direct struct tags

## Next Slice

- reduce the still-failing `bool_constant<T(-1) < T(0)>` case when the owning
  template also carries a defaulted NTTP such as
  `bool = is_arithmetic<T>::value`
- decide whether that remaining blocker belongs in alias-template application,
  deferred template-argument materialization, or top-level template-struct
  bookkeeping before the next implementation change
- use the new reduced reproducer in
  `tests/cpp/internal/negative_case/namespaced_inherited_type_alias_base_member_lookup_parse.cpp`
  as the starting point instead of reopening the full EASTL stack

## Blockers

- `eastl_integer_sequence_simple.cpp` and
  `eastl_type_traits_simple.cpp` now stop with
  `object has incomplete type: eastl::is_signed_helper`
- the surviving reduced failure is narrower than the original EASTL stack:
  `template<typename T, bool = arithmetic<T>::value> struct helper : bool_constant<T(-1) < T(0)> {};`
  still trips the same incomplete-type error even after the aliased-base
  `::type` fix
- the failure no longer requires the full EASTL headers: the same incomplete
  type now reproduces in
  `tests/cpp/internal/negative_case/namespaced_inherited_type_alias_base_member_lookup_parse.cpp`
  when the helper is referenced as `typename eastl::is_signed_helper<T>::type`
  inside a declaration
- `eastl_vector_simple.cpp` now stops at
  `ref/EASTL/include/EASTL/internal/function_detail.h:237:16` with
  `unexpected token in expression: .`
- `eastl_memory_simple.cpp` and `eastl_tuple_simple.cpp` still need a fresh
  post-fix reclassification before Step 3 resumes

## Resume Notes

- activation completed with no prior active `plan.md` or `plan_todo.md`
- the open idea remains the authoritative scope source
- Step 1 inventory is now captured in `tests/cpp/eastl/README.md`
- the foundation batch now has executable parse recipes in
  `tests/cpp/internal/InternalTests.cmake`
- `tests/cpp/internal/parse_only_case/record_member_underlying_type_parse.cpp`
  is the reduced internal regression for the fixed parser blocker
- `tests/cpp/internal/parse_only_case/record_base_variable_template_value_arg_parse.cpp`
  is the reduced internal regression for the nested variable-template parser
  blocker in record bases
- `tests/cpp/internal/postive_case/inherited_type_alias_base_member_lookup_parse.cpp`
  now covers the inherited aliased-base member-typedef path that was fixed
- the remaining `is_signed_helper` blocker no longer depends on inherited
  `false_type` lookup alone; it requires the combination of a defaulted NTTP
  and an alias-template base carrying a dependent expression argument
- a smaller negative regression now captures that remaining shape without the
  full EASTL include stack:
  `tests/cpp/internal/negative_case/namespaced_inherited_type_alias_base_member_lookup_parse.cpp`
- `eastl_piecewise_construct_simple.cpp` and
  `eastl_tuple_fwd_decls_simple.cpp` parse successfully but first fail during
  canonical/sema expansion with undeclared identifiers from EASTL internals
