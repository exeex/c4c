# EASTL Container Bring-Up Todo

Status: Active
Source Idea: ideas/open/eastl_container_bringup_plan.md
Source Plan: plan.md

## Active Item

- Step 3: Object lifetime and tuple layer
- Current slice: reduce and fix the shared parser frontier from
  `ref/EASTL/include/EASTL/internal/function.h` by determining whether the
  pinned `Base::operator=(other);` / `object has incomplete type:
  eastl::internal::function_detail` cluster is still parser-induced or the next
  real semantic/canonical blocker

## Todo

- [x] Step 1: Inventory and recipe normalization
- [x] Step 2: Foundation headers and traits
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
- [x] Registered namespaced alias-template metadata under the qualified alias
  spelling and taught alias-template argument parsing to respect alias NTTP
  parameter kinds, so `bool_constant<T(-1) < T(0)>` now parses in namespaced
  inherited-base reductions
- [x] Promoted
  `tests/cpp/internal/postive_case/namespaced_inherited_type_alias_base_member_lookup_parse.cpp`
  to a positive parse regression for the unparenthesized
  `bool_constant<T(-1) < T(0)>` shape under `namespace eastl`
- [x] Reclassified the affected EASTL parse recipes after the parser fix:
  `eastl_integer_sequence_simple.cpp` and `eastl_type_traits_simple.cpp` now
  stop on `eastl::internal::is_complete_type__spec_167` instead of
  `eastl::is_signed_helper`
- [x] Advanced `eastl_integer_sequence_simple.cpp` and
  `eastl_type_traits_simple.cpp` through `--parse-only`; both now join the
  existing Step 2 canonical/sema undeclared-identifier cluster headed by
  `mPart0` / `mPart1` in instantiated EASTL internals
- [x] Step 2 completion check: all five foundation targets now either parse
  successfully and fail later in canonical/sema or were already narrowed to
  explicit later-stage blockers
- [x] Added
  `tests/cpp/internal/postive_case/qualified_template_operator_assign_expr_parse.cpp`
  as a reduced parser regression for qualified `TemplateId::operator=(...)`
  calls inside pack expansions, and taught the expression parser to accept
  `::operator...` after a template-id owner in expression context
- [x] Reclassified the Step 3 fronts after the qualified-operator parser fix:
  `eastl_tuple_simple.cpp` no longer stops at
  `ref/EASTL/include/EASTL/tuple.h:346`, and both Step 3 targets now advance
  into later parser pressure under libstdc++ / EASTL internals instead of the
  old `TupleLeaf<...>::operator=` expression failure
- [x] Reduced the shared `function_detail.h` parser blocker to
  `tests/cpp/internal/postive_case/eastl_function_detail_allocator_member_call_parse.cpp`
  and taught block-scope statement parsing to prefer expression dispatch for
  `value.member(...)` / `value->member(...)` when a visible value shadows a
  same-spelled type name
- [x] Reclassified `eastl_vector_simple.cpp` after the parser fix: the old
  `ref/EASTL/include/EASTL/internal/function_detail.h:237:16`
  `unexpected token in expression: .` failure is gone, and the current pinned
  frontier is now the later incomplete-type cluster in
  `ref/EASTL/include/EASTL/internal/function.h`
- [x] Added
  `tests/cpp/internal/postive_case/qualified_typedef_operator_assign_expr_parse.cpp`
  as a reduced parse-only guardrail for typedef-alias-owned
  `Base::operator=(other);` statements and taught statement parsing to route
  `Type::operator...` shapes into expression parsing instead of local
  declaration parsing
- [x] Re-ran the focused EASTL/vector and qualified-operator guardrails plus
  full-suite before/after logs; the tree stayed monotonic with the same three
  pre-existing failures and one additional passing regression test

## Next Slice

- reduce the new post-`function_detail.h` Step 3/4 frontier from the remaining
  `eastl_memory_simple.cpp` and `eastl_tuple_simple.cpp` parse-time stalls
  under the deeper libstdc++ / EASTL stack
- determine whether `eastl_vector_simple.cpp`'s new
  `ref/EASTL/include/EASTL/internal/function.h` incomplete-type diagnostics are
  a semantic/canonical blocker or another parser-induced partial-definition
  artifact now that typedef-owner qualified operator statements no longer route
  through local-declaration parsing
- keep
  `tests/cpp/internal/postive_case/qualified_template_operator_assign_expr_parse.cpp`
  green as the parser-side guardrail for the fixed Step 3 expression shape
- keep
  `tests/cpp/internal/postive_case/qualified_typedef_operator_assign_expr_parse.cpp`
  green as the new parser-side guardrail for typedef-owned qualified operator
  calls
- keep
  `tests/cpp/internal/postive_case/eastl_function_detail_allocator_member_call_parse.cpp`
  green as the guardrail for the fixed block-scope shadowed-member-call shape

## Blockers

- `eastl_integer_sequence_simple.cpp` and
  `eastl_type_traits_simple.cpp` now pass `--parse-only` and fail later during
  canonical/sema with undeclared identifiers such as `mPart0`, `mPart1`, `i`,
  and `bBit`
- the earlier reduced `is_signed_helper` failure is no longer a blocker; the
  same namespaced defaulted-NTTP plus inherited-alias-base shape now parses in
  `tests/cpp/internal/postive_case/namespaced_inherited_type_alias_base_member_lookup_parse.cpp`
- `eastl_vector_simple.cpp` now stops at
  `ref/EASTL/include/EASTL/internal/function.h:66:26` with
  `object has incomplete type: eastl::internal::function_detail` before the
  parse-only timeout
- the reduced typedef-owner `Base::operator=(other);` parser shape is now fixed
  at statement dispatch, but the standalone AST dump still prints it in a
  declaration-like form, so more parser/canonical investigation may still be
  needed if the EASTL `function.h` frontier does not move
- `eastl_memory_simple.cpp` and `eastl_tuple_simple.cpp` no longer stop on the
  old `TupleLeaf<...>::operator=` or `function_detail.h` parser failures, but
  they still time out under deeper libstdc++ / EASTL parser pressure
- additional reduction work ruled out simpler namespace-qualified paths:
  `ns::wrap<int> value{}`, `ns::wrap<int>::type value{}`, and
  `ns::wrap<int>` with a defaulted NTTP all parse successfully, so the
  surviving blocker still depends on the combination of a defaulted NTTP and
  inherited alias-base member lookup rather than namespace qualification alone
- the current failure also reproduces without the explicit `eastl::` qualifier
  inside the namespace body; `typename is_signed_helper<T>::type value{}`
  still reaches the incomplete-type error when the helper stays under
  `namespace eastl`
- a temporary parser-side probe confirmed the old failure reached the
  incomplete-object check as a bare `eastl::is_signed_helper` type with no
  deferred member metadata; after the qualified alias-template registration fix
  that probe is no longer needed and has been removed

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
- `tests/cpp/internal/postive_case/qualified_template_operator_assign_expr_parse.cpp`
  now covers the Step 3 qualified `TemplateId::operator=(...)` expression shape
  that previously blocked `eastl_tuple_simple.cpp`
- `tests/cpp/internal/postive_case/eastl_function_detail_allocator_member_call_parse.cpp`
  now covers the block-scope `allocator.deallocate(...)` expression statement
  that previously misparsed as a declaration after `func->~Functor()`
- `tests/cpp/internal/postive_case/qualified_typedef_operator_assign_expr_parse.cpp`
  now covers typedef-owned qualified operator calls in statement position
- the remaining `is_signed_helper` blocker no longer depends on inherited
  `false_type` lookup alone; it requires the combination of a defaulted NTTP
  and an alias-template base carrying a dependent expression argument
- that parser shape is now covered by the positive regression:
  `tests/cpp/internal/postive_case/namespaced_inherited_type_alias_base_member_lookup_parse.cpp`
- `eastl_integer_sequence_simple.cpp` and `eastl_type_traits_simple.cpp` now
  sit on the same later-stage sema cluster as
  `eastl_piecewise_construct_simple.cpp` and
  `eastl_tuple_fwd_decls_simple.cpp`
- probe reductions showed that namespace-qualified template instantiation and
  namespace-qualified `::type` lookup both work in simpler cases; the current
  failure still needs the inherited-base path exposed by `is_signed_helper`
- a further reduction showed that the declaration-context `typename X<T>::type`
  shape itself now works in both global and namespaced simple wrappers; the
  blocker only reappears once the helper is namespace-scoped and carries the
  defaulted-NTTP plus inherited-alias base pattern
- `eastl_piecewise_construct_simple.cpp` and
  `eastl_tuple_fwd_decls_simple.cpp` parse successfully but first fail during
  canonical/sema expansion with undeclared identifiers from EASTL internals
