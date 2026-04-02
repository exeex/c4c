# EASTL Container Bring-Up Todo

Status: Active
Source Idea: ideas/open/eastl_container_bringup_plan.md
Source Plan: plan.md

## Active Item

- Step 3: Object lifetime and tuple layer
- Current slice: use the later libstdc++ `type_traits` reach earned by the new
  simple known-type template-argument fast path to cut the next smaller
  `eastl_memory_simple.cpp` / `eastl_tuple_simple.cpp` timeout reduction from
  the shared post-`_FromElementType` / `typename ...` trait machinery

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
- [x] Added
  `tests/cpp/internal/postive_case/qualified_typedef_operator_assign_runtime.cpp`
  as a behavior-level regression for typedef-owner qualified `operator=`
  statements so the parser/HIR path now proves the base assignment actually
  executes instead of only surviving `--parse-only`
- [x] Taught block-scope statement disambiguation to route
  `Type::operator...(...)` shapes into expression parsing before the generic
  qualified-type probe, and taught qualified operator references inside method
  bodies to collapse visible typedef-owner call spellings back onto the
  existing implicit-`this` member-call path
- [x] Taught implicit method-call lowering to resolve inherited base methods via
  recursive struct-method lookup instead of only probing the immediate class,
  so unqualified operator calls recovered from typedef-owner spellings no
  longer lower as unresolved free functions
- [x] Reclassified `eastl_vector_simple.cpp` after the qualified-operator/HIR
  fix: the old
  `ref/EASTL/include/EASTL/internal/function.h:66:26`
  `object has incomplete type: eastl::internal::function_detail` frontier is
  gone, and the current `--parse-only` / `--dump-canonical` workflows now time
  out under deeper libstdc++ / EASTL pressure with no replacement terminal
  diagnostic yet
- [x] Re-ran the focused qualified-operator guardrails plus full `ctest -j8`;
  the tree still ends on the same three pre-existing failures
  (`positive_sema_linux_stage2_repro_03_asm_volatile_c`,
  `backend_lir_adapter_aarch64_tests`, and
  `llvm_gcc_c_torture_src_20080502_1_c`)
- [x] Added file-aware parser-debug progress coverage with
  `cpp_std_vector_parse_debug_progress_file` and taught parser progress
  heartbeats to print the active token's logical source file so the remaining
  EASTL timeout-only Step 3/4 pressure can be localized to concrete headers
- [x] Added
  `tests/cpp/internal/postive_case/qualified_variable_template_compare_parse.cpp`
  as a parse-only guardrail for qualified variable-template comparisons in
  expression context and tightened the functional-cast probe so template-id
  expressions with obvious non-call tokens after `>` stop paying the full
  tentative type-parse cost
- [x] Re-ran the focused qualified-type / EASTL parser guardrails plus
  full-suite before/after logs; the regression guard passed monotonically with
  `2694 -> 2695` passes and the same three pre-existing failures
  (`positive_sema_linux_stage2_repro_03_asm_volatile_c`,
  `backend_lir_adapter_aarch64_tests`, and
  `llvm_gcc_c_torture_src_20080502_1_c`)
- [x] Added
  `tests/cpp/internal/postive_case/qualified_trait_value_template_arg_parse.cpp`
  as a parse-only guardrail for alias-template NTTPs that start with
  `Trait<...>::value`, and taught both template-argument disambiguation and the
  shared type-start probe to treat that shape as value-like instead of paying
  the full speculative type parse first
- [x] Re-ran the focused trait-value guardrails plus 12s EASTL parser-debug
  probes: `eastl_memory_simple.cpp`, `eastl_tuple_simple.cpp`, and
  `eastl_vector_simple.cpp` still time out, but the old
  `/usr/include/c++/14/type_traits` `__and_<...>::value` / `add_const_t`
  frontier is no longer the terminal hotspot and the remaining work now lands
  deeper in shared trait / alias / constexpr machinery
- [x] Re-ran the full suite and monotonic regression guard; the tree stayed
  monotonic with `2694 -> 2696` passes and the same three pre-existing failures
  (`positive_sema_linux_stage2_repro_03_asm_volatile_c`,
  `backend_lir_adapter_aarch64_tests`, and
  `llvm_gcc_c_torture_src_20080502_1_c`)
- [x] Taught `try_parse_template_type_arg()` to fast-path simple known type
  heads (`T`, resolved typedefs, and `typename`-started dependent names)
  through `parse_base_type()` before falling back to the full
  `parse_type_name()` probe, trimming repeated declarator work in the shared
  EASTL / libstdc++ template stack
- [x] Added
  `tests/cpp/internal/postive_case/template_known_type_arg_fast_path_parse.cpp`
  as a stable parse regression for the new simple known-type template-argument
  fast path
- [x] Re-ran the focused parser guardrails plus 12s Step 3 parser-debug probes:
  `eastl_memory_simple.cpp` now reaches later `/usr/include/c++/14/type_traits`
  work through `_FromElementType` / later `typename` paths, and
  `eastl_tuple_simple.cpp` now reaches deeper `type_traits` pressure through
  `typename` template arguments and later initializers before timing out
- [x] Re-ran the full suite and monotonic regression guard after the fast-path
  slice; the tree improved monotonically with `2696 -> 2697` passes and the
  same three pre-existing failures
  (`positive_sema_linux_stage2_repro_03_asm_volatile_c`,
  `backend_lir_adapter_aarch64_tests`, and
  `llvm_gcc_c_torture_src_20080502_1_c`)

## Next Slice

- reduce the post-trait-value Step 3 frontier from the remaining
  `eastl_memory_simple.cpp` and `eastl_tuple_simple.cpp` parse-time stalls,
  starting from the new later libstdc++ hotspots now reached around
  `/usr/include/c++/14/type_traits:1542`,
  `/usr/include/c++/14/type_traits:2654`,
  `/usr/include/c++/14/type_traits:2674`, and
  `/usr/include/c++/14/type_traits:3308`
- reduce the timeout-only `eastl_vector_simple.cpp` frontier from the later
  EASTL-side hotspots now reached in `EASTL/internal/fill_help.h`,
  `EASTL/functional.h`, and `EASTL/meta.h`, then capture the next smaller
  parser/canonical blocker from that deeper stack
- keep
  `tests/cpp/internal/postive_case/qualified_template_operator_assign_expr_parse.cpp`
  green as the parser-side guardrail for the fixed Step 3 expression shape
- keep
  `tests/cpp/internal/postive_case/qualified_typedef_operator_assign_expr_parse.cpp`
  green as the new parser-side guardrail for typedef-owned qualified operator
  calls
- keep
  `tests/cpp/internal/postive_case/qualified_typedef_operator_assign_runtime.cpp`
  green as the behavior-level guardrail for typedef-owner qualified operator
  calls lowering through inherited base methods
- keep
  `tests/cpp/internal/postive_case/eastl_function_detail_allocator_member_call_parse.cpp`
  green as the guardrail for the fixed block-scope shadowed-member-call shape
- keep
  `tests/cpp/internal/postive_case/qualified_trait_value_template_arg_parse.cpp`
  green as the guardrail for the new `Trait<...>::value` template-argument /
  type-start disambiguation shortcut

## Blockers

- `eastl_integer_sequence_simple.cpp` and
  `eastl_type_traits_simple.cpp` now pass `--parse-only` and fail later during
  canonical/sema with undeclared identifiers such as `mPart0`, `mPart1`, `i`,
  and `bBit`
- the earlier reduced `is_signed_helper` failure is no longer a blocker; the
  same namespaced defaulted-NTTP plus inherited-alias-base shape now parses in
  `tests/cpp/internal/postive_case/namespaced_inherited_type_alias_base_member_lookup_parse.cpp`
- `eastl_vector_simple.cpp` no longer emits the old
  `ref/EASTL/include/EASTL/internal/function.h:66:26`
  `object has incomplete type: eastl::internal::function_detail` diagnostic,
  but both `--parse-only` and `--dump-canonical` now time out under deeper
  libstdc++ / EASTL pressure with no replacement reduced testcase yet
- `eastl_memory_simple.cpp` and `eastl_tuple_simple.cpp` no longer stop on the
  old `TupleLeaf<...>::operator=` or `function_detail.h` parser failures, but
  they still time out under deeper libstdc++ / EASTL parser pressure
- the new simple known-type template-argument fast path moved the Step 3
  parser-debug frontier later inside `/usr/include/c++/14/type_traits`, but it
  did not yet produce a smaller standalone reduction for the remaining timeout
  path
- the new file-aware parser-debug heartbeat confirms the current
  `eastl_memory_simple.cpp` timeout path reaches
  `ref/EABase/include/Common/EABase/int128.h`,
  `ref/EASTL/include/EASTL/internal/type_transformations.h`,
  `ref/EASTL/include/EASTL/internal/type_properties.h`, and
  `ref/EASTL/include/EASTL/internal/type_pod.h` within the first 5 seconds,
  but the exact smaller reduction inside that chain is still pending
- the new functional-cast cutoff moves both Step 3 timeout probes deeper into
  later shared headers instead of stalling as early on qualified
  variable-template expressions; current 12s parser-debug progress reaches
  `ref/EASTL/include/EASTL/utility.h`,
  `ref/EASTL/include/EASTL/internal/functional_base.h`, and
  `/usr/include/c++/14/type_traits` before timing out
- the new trait-value shortcut moves the current timeout frontier past the old
  `__and_<...>::value` / `add_const_t` cluster, but `eastl_memory_simple.cpp`
  still times out after reaching deeper `type_traits` work around
  `is_arithmetic`, `is_trivially_constructible`, and nearby `_Tp`-driven trait
  aliases
- `eastl_tuple_simple.cpp` still times out after reaching deeper
  `/usr/include/c++/14/type_traits` work; the current 12s heartbeat now reaches
  later `typename`-started template arguments near `:2674` and initializer work
  near `:3308` before the timeout
- `eastl_vector_simple.cpp` still times out, but the current 12s heartbeat now
  reaches later EASTL-side parser pressure in
  `ref/EASTL/include/EASTL/internal/fill_help.h`,
  `ref/EASTL/include/EASTL/functional.h`, and
  `ref/EASTL/include/EASTL/meta.h`
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
- `tests/cpp/internal/postive_case/qualified_typedef_operator_assign_runtime.cpp`
  now proves the typedef-owner qualified operator statement survives parsing and
  lowers through the inherited base method instead of an unresolved free
  function symbol
- `tests/cpp/internal/postive_case/qualified_trait_value_template_arg_parse.cpp`
  now covers alias-template NTTPs that begin with `Trait<...>::value`, and the
  shared helper in `src/frontend/parser/types_helpers.hpp` also prevents that
  shape from re-entering the broad C++ type-start fallback
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
- the current full-suite post-change baseline lives in `test_fail_after.log`
  and still reports the same three pre-existing failing tests noted above; the
  repaired monotonic guard comparison against `test_fail_before.log` passes for
  this slice with one additional passing test and no newly failing tests
- parser-debug progress heartbeats now include the active logical source file;
  the new diagnostic-format guardrail is
  `cpp_std_vector_parse_debug_progress_file`
- with the new heartbeat format, a 6-second `eastl_memory_simple.cpp`
  `--parser-debug --parse-only` repro now shows the timeout path moving through
  `ref/EABase/include/Common/EABase/int128.h`,
  `ref/EASTL/include/EASTL/internal/type_transformations.h`,
  `ref/EASTL/include/EASTL/internal/type_properties.h`, and
  `ref/EASTL/include/EASTL/internal/type_pod.h`
- `tests/cpp/internal/postive_case/qualified_variable_template_compare_parse.cpp`
  now covers the shared variable-template comparison shape trimmed out of the
  functional-cast probe while reducing the remaining Step 3 timeout pressure
- `tests/cpp/internal/postive_case/template_known_type_arg_fast_path_parse.cpp`
  now covers the simple known-type template-argument shapes accelerated by the
  new fast path in `try_parse_template_type_arg()`
