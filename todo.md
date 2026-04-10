# EASTL Container Bring-Up Todo

Status: Active
Source Idea: ideas/open/47_eastl_container_bringup_plan.md
Source Plan: plan.md

## Active Item

- Step 3: keep `eastl_type_traits_simple.cpp` as the active EASTL frontier, but
  treat the next remaining workflow mismatch as the bounded `remove_cv`
  trait-follow-up rather than the now-fixed inherited `is_enum` path.
- Iteration focus: keep the focused inherited `integral_constant<bool,
  __is_enum(T)>` runtime coverage current, preserve the deferred-NTTP builtin
  trait evaluation fix, and resume the EASTL workflow from the later
  `remove_cv_t<const volatile unsigned int>` mismatch.
- Iteration target: record that the old `exit 3` enum-trait failure is gone,
  the standalone EASTL workflow now reaches `exit 7`, and the next slice should
  reduce `remove_cv_t<const volatile unsigned int>` to a smallest generic repro
  before touching unrelated EASTL cases.
- Reduced repro: the focused internal runtime case
  `template_builtin_is_enum_inherited_value_runtime.cpp` now passes, proving
  that inherited `integral_constant<bool, __is_enum(T)>::value` bases preserve
  enum classification during template instantiation, while the standalone EASTL
  workflow now exits `7` at the later `remove_cv_t` check.
- Current blocker: `eastl::remove_cv_t<const volatile unsigned int>` still
  fails in the standalone workflow after the inherited `is_enum` path was
  repaired. Treat that as the next bounded generic trait-family slice.

## Completed

- Added focused runtime coverage in
  `tests/cpp/internal/postive_case/template_builtin_is_enum_inherited_value_runtime.cpp`
  so inherited `integral_constant<bool, __is_enum(T)>::value` lookups stay
  covered as a standalone generic repro.
- Taught deferred NTTP expression evaluation to fold builtin type traits such
  as `__is_enum(T)` while instantiating template bases, which repairs the
  reduced inherited enum-trait repro and the old `eastl_type_traits_simple`
  `exit 3` mismatch.
- Re-ran the focused inherited enum-trait runtime regression, the
  `cpp_eastl_type_traits_parse_recipe` workflow coverage, and `cmake --build
  build --target eastl_type_traits_simple_workflow -j8`; the standalone EASTL
  workflow now advances from `exit 3` to `exit 7`, so the next blocker is
  `remove_cv_t<const volatile unsigned int>`.
- Re-ran the full `ctest --test-dir build -j8 --output-on-failure` suite and
  compared `test_fail_before.log` vs `test_fail_after.log`; the regression
  guard passed monotonically at 3297/3299 passing tests versus the earlier
  3289/3297 baseline, with zero newly failing tests. The remaining failures are
  still `cpp_eastl_utility_parse_recipe` and
  `cpp_eastl_memory_uses_allocator_parse_recipe`.
- Added focused runtime coverage in
  `tests/cpp/internal/postive_case/template_variable_alias_member_typedef_runtime.cpp`
  and confirmed the reduced alias-backed `enable_if_t` / `conditional_t`
  variable-template repro now passes.
- Added focused runtime coverage in
  `tests/cpp/internal/postive_case/template_variable_alias_inherited_member_typedef_runtime.cpp`
  so the `decltype(...)->type_identity<T&>` owner chain behind
  `add_lvalue_reference_t<int>` stays covered too.
- Taught compile-time `is_reference` evaluation to treat instantiated
  `add_lvalue_reference` / `add_rvalue_reference` owners as reference
  transforms even when the inherited member typedef is not fully materialized
  yet, which repairs the reduced inherited-owner repro and the old final
  `eastl_type_traits_simple` `exit 22` mismatch.
- Re-ran the focused alias-member runtime regression, the new inherited-owner
  runtime regression, and `cmake --build build --target
  eastl_type_traits_simple_workflow -j8`; the standalone EASTL workflow now
  advances from `exit 22` to `exit 3`, so the next blocker is
  `eastl::is_enum<Color>::value`.
- Re-ran the full `ctest --test-dir build -j8 --output-on-failure` suite and
  compared `test_fail_before.log` vs `test_fail_after.log`; the regression
  guard passed monotonically at 3296/3298 passing tests versus the earlier
  3289/3297 baseline, with zero newly failing tests.
- Extended
  `tests/cpp/internal/postive_case/template_variable_alias_member_typedef_runtime.cpp`
  to cover the direct alias-backed `add_lvalue_reference_t<int>` /
  `is_reference_v` path too, and fixed parser alias-member substitution so
  outer reference qualifiers on member typedef spellings like `using type = T&`
  survive replacement with the bound concrete type.
- Re-ran the focused runtime regression plus the full `ctest --test-dir build
  -j8 --output-on-failure` suite; the regression guard is monotonic at
  3295/3297 passing tests versus the earlier 3289/3297 baseline, with zero new
  failing tests.
- Confirmed `cmake --build build --target eastl_type_traits_simple_workflow -j8`
  still exits `22`, so the remaining type-trait work is broader than the
  direct alias-member substitution repaired in this slice.
- Fixed parser alias-template application to accept omitted trailing default
  template arguments, which repairs partial-specialization heads such as
  `trait<T, enable_if_t<true>>` and unblocks `eastl_type_traits_simple.cpp`
  from failing during parse with `object has incomplete type`.
- Fixed parser and HIR template-specialization registration so namespace-scoped
  partial specializations keyed by unqualified `template_origin_name` still
  resolve against their fully qualified primaries during alias-member lookup.
- Fixed HIR template static-member evaluation for namespace-scoped
  unqualified-owner lookups and stopped re-lowering duplicate mangled
  variable-template globals from overwriting earlier correct instances.
- Confirmed `tests/cpp/eastl/eastl_type_traits_simple.cpp --parse-only` now
  succeeds again and the standalone workflow advances from the old frontend
  failure into the final runtime-only mismatch at exit `22`.
- Added focused runtime coverage in
  `tests/cpp/internal/postive_case/scoped_enum_bitwise_runtime.cpp` so scoped
  enums stay covered through parsing, canonicalization, and codegen.
- Added focused parse-only coverage in
  `tests/cpp/internal/parse_only_case/template_alias_empty_pack_default_arg_parse.cpp`
  and taught alias-template metadata/application to accept empty type packs in
  default template arguments, so `void_t<>` no longer regresses enclosing
  template parameter lists with `expected GREATER but got 'struct'`.
- Fixed parser enum handling so `enum class` / scoped enums are accepted as
  real enums instead of degrading into a fake `StructDef(struct ...)`, while
  also registering enum type names for later lookup.
- Confirmed the focused scoped-enum runtime regression passes, the full suite
  remains monotonic at 3294/3294 passing tests versus the earlier
  3293/3293 baseline, and the old `std::byte` LLVM verifier failure in
  `eastl_type_traits_simple_workflow` is gone.
- Re-ran `cmake --build build --target eastl_type_traits_simple_workflow -j8`
  and confirmed the active type-traits frontier moved forward again: the host
  binary still exits `0`, but the c4c-built binary now exits `10`, so the next
  slice is a runtime semantics mismatch rather than parser/backend corruption.
- Reduced the `eastl_memory_uses_allocator_frontier.cpp` parser timeout to the
  unsupported structured-binding bridge enabled by our predefined macro
  surface: defining `EA_COMPILER_NO_STRUCTURED_BINDING` makes both
  `EASTL/utility.h` and `EASTL/tuple.h` parse comfortably again.
- Confirmed the frontend still rejects real structured binding declarations
  (`auto [a, b] = ...`) while EASTL/EABase was enabling the optional
  structured-binding bridge based only on `__cplusplus >= 201703L`.
- Added focused preprocessor coverage in
  `tests/preprocessor/frontend_cxx_preprocessor_tests.cpp` and now predefine
  `EA_COMPILER_NO_STRUCTURED_BINDING` for the C++ source profiles until the
  frontend actually supports that language feature.
- Reduced the next shared EASTL sema blocker to namespaced out-of-class method
  owner context: record definitions under namespace scope kept fully qualified
  tags like `eastl::allocator`, while out-of-class method definitions inside
  that namespace still arrived as `allocator::operator=` and `allocator::read`.
- Added focused frontend coverage in
  `tests/cpp/internal/postive_case/namespaced_out_of_class_method_context_frontend.cpp`
  and fixed sema owner-tag resolution so out-of-class methods recover the known
  record tag for implicit `this`, unqualified member lookup, and
  `return *this;`.
- Confirmed the focused regression passes, `#include <EASTL/allocator.h>` now
  completes under `--dump-canonical`, and `tests/cpp/eastl/eastl_memory_simple.cpp`
  no longer reports `ref/EASTL/include/EASTL/allocator.h:210:16`.
- Re-ran nearby member-context regressions plus the full suite; the regression
  guard remains monotonic at 3289/3289 passing tests versus the earlier
  3288/3288 baseline, with zero new failing tests.
- Reduced the next vector-only canonical blocker to a generic free
  `operator==` / `operator!=` overload-set failure and added focused frontend
  coverage in
  `tests/cpp/internal/postive_case/free_operator_eq_overload_frontend.cpp`.
- Fixed sema's top-level C++ overload-set admission so normalized
  `operator_*` function names are treated as overloadable rather than rejected
  as conflicting C-style redeclarations.
- Confirmed the new focused frontend regression passes, the reduced global
  free-operator repro canonicalizes, and
  `tests/cpp/eastl/eastl_vector_simple.cpp --dump-canonical` now moves past
  the old `operator_eq` / `operator_neq` errors into the allocator-only
  follow-up at `EASTL/allocator.h:210:16` and
  `EASTL/vector.h:438:61`.
- Re-ran `tests/cpp/eastl/eastl_memory_simple.cpp --dump-canonical` plus the
  full `ctest --test-dir build -j8 --output-on-failure` suite; the regression
  guard remains monotonic at 3291/3291 passing tests versus the earlier
  3278/3278 baseline, with zero new failing tests.
- Converted
  `cpp_eastl_memory_uses_allocator_timeout_baseline` into the positive workflow
  test `cpp_eastl_memory_uses_allocator_parse_recipe`, and confirmed
  `tests/cpp/eastl/eastl_memory_uses_allocator_frontier.cpp` now completes
  through both `--parse-only` and `--dump-canonical`.
- Confirmed `tests/cpp/eastl/eastl_memory_simple.cpp --parse-only` now succeeds
  too; the full memory driver has moved from a parse timeout into a later
  canonical-timeout stage, while `eastl_vector_simple.cpp` remains the next
  parse-timeout frontier.
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
- Fixed sema diagnostic source locations so EASTL follow-up failures now point
  at the real libstdc++ / EABase headers instead of fake line numbers in the
  tiny driver TU.
- Fixed placement `operator new[]` / `operator delete[]` overload handling so
  `/usr/include/c++/14/new` no longer trips `conflicting types` during EASTL
  bring-up.
- Fixed inherited implicit-member lookup in out-of-class method bodies so the
  old `mPart0` / `mPart1` cluster in `EABase/int128.h` is gone.
- Reduced block-scope direct-initialization regressions (`int i(0)`,
  `const bool b(x > 0)`, `Box copy(source)`) and fixed both parser and HIR
  lowering so those locals now lower as real initialized variables.
- Confirmed `tests/cpp/eastl/eastl_tuple_simple.cpp` parse-only now succeeds
  again, and the old `int128.h` local-direct-init undeclared-identifier cluster
  (`tempDivisor`, `bBit`, `bValue1IsPositive`, `bANegative`, etc.) is gone.
- Reduced the `/usr/include/c++/14/cstddef` lvalue-reference binding failures
  to assignment expressions incorrectly losing lvalue category.
- Added focused regression coverage in
  `tests/cpp/internal/postive_case/assignment_expr_return_lvalue_ref_runtime.cpp`
  and fixed sema, HIR, and LIR handling so assignment expressions remain usable
  as `T&` results and reference initializers.
- Confirmed `tests/cpp/eastl/eastl_tuple_simple.cpp` now moves past the old
  `/usr/include/c++/14/cstddef` blocker and fails later in
  `/usr/include/c++/14/bits/ranges_util.h` with undeclared identifiers such as
  `bidirectional_iterator`, `_M_begin`, `_M_end`, `_M_size`, and `this`.
- Reduced the `if constexpr (bidirectional_iterator<T>)` concept-id path to a
  focused internal frontend repro and fixed parser/sema/HIR handling so tagged
  concept-id expressions fold as constant booleans instead of lowering as
  untyped declrefs.
- Added focused frontend coverage in
  `tests/cpp/internal/postive_case/cpp20_if_constexpr_concept_id_frontend.cpp`
  and confirmed both the reduced concept-id repro and the templated member
  variant now compile.
- Confirmed `tests/cpp/eastl/eastl_tuple_simple.cpp` no longer reports
  `bidirectional_iterator` from `/usr/include/c++/14/bits/ranges_util.h`; the
  active tuple blocker is now the narrower member-context cluster around
  `_M_begin`, `_M_end`, `_M_size`, `_S_store_size`, and `this`.
- Reduced one bounded subcase of that `ranges_util.h` member-context frontier
  into focused frontend coverage in
  `tests/cpp/internal/postive_case/template_inline_method_member_context_frontend.cpp`
  and patched sema so inline class-template methods recover enclosing owner
  NTTP bindings, implicit member lookup, and `this` during validation.
- Confirmed the new regression passes and the full `ctest --test-dir build -j`
  suite remains monotonic, now at 3280/3280 passing tests versus the earlier
  3278/3278 baseline.
- Reduced the remaining `ranges_util.h` follow-up further in scratch repros:
  a minimal constrained class-template method case
  (`template<C T> struct Box { Box& f(); }; Box<int> b; b.f();`) still lowers
  `b` as the primary `Box` rather than an instantiated record, failing later
  with `StmtEmitter: field 'f' not found in struct/union 'Box'`.
- Promoted that constrained concept-shorthand class-template method failure
  into focused frontend coverage in
  `tests/cpp/internal/postive_case/constrained_template_method_call_frontend.cpp`
  and fixed template-parameter parsing so `template<C T>` no longer
  misclassifies constrained type parameters as NTTPs.
- Confirmed the focused constrained-template frontend test passes, the scratch
  `Box<int>` reproducer now instantiates and lowers correctly, and the full
  regression guard remains monotonic at 3281/3281 passing tests versus the
  earlier 3280/3280 baseline.
- Reduced the remaining `ranges::subrange` member-context failure further to a
  focused constrained member-template conversion-operator reproducer: inside a
  class template, `template<C Pair> constexpr operator Pair() const` was
  ejecting later members out of the record body, which then broke follow-up
  member parsing and lowering.
- Added focused frontend coverage in
  `tests/cpp/internal/postive_case/constrained_member_conversion_operator_frontend.cpp`
  and fixed record-member parsing so prefixed conversion operators following
  constrained member-template preludes stay attached to the class body.
- Confirmed the reduced constrained conversion-operator repro now parses and
  lowers correctly again, and `tests/cpp/eastl/eastl_tuple_simple.cpp` moves
  past the old `/usr/include/c++/14/bits/ranges_util.h` undeclared-identifier
  cluster into a later semantic failure: `initializer for scalar member 'first'
  must have exactly one argument`.
- Reduced that member-init semantic follow-up into focused internal runtime
  regressions covering both typedef-backed record members
  (`ctor_init_member_typedef_ctor_runtime.cpp`) and zero-argument scalar
  value-init members (`ctor_init_member_default_value_init_runtime.cpp`).
- Fixed template-struct method cloning so instantiated records preserve
  constructor / defaulted / ref-qualified metadata and constructor initializer
  lists, allowing HIR to register concrete constructors such as `Pair_T_int`.
- Fixed constructor member-init lowering so field types recover resolved
  semantic types from the owning record field declaration before constructor
  dispatch, and zero-argument member initializers now value-initialize scalar
  members instead of failing as malformed scalar ctor calls.
- Confirmed both focused ctor-init regressions pass, and
  `tests/cpp/eastl/eastl_tuple_simple.cpp` no longer reports
  `initializer for scalar member 'first' must have exactly one argument`;
  `--parse-only` succeeds and `--dump-canonical` now times out later in the
  deeper tuple/libstdc++ stack.
- Re-baselined the smaller EASTL canonical cases and confirmed
  `eastl_piecewise_construct_simple.cpp`,
  `eastl_tuple_fwd_decls_simple.cpp`, and
  `eastl_integer_sequence_simple.cpp` all now complete under
  `--dump-canonical`, so the README inventory had gone stale.
- Measured the parser side of the current tuple/utility frontier:
  `eastl_utility_simple.cpp --parse-only` completes in about `10.476s`,
  `eastl_tuple_simple.cpp --parse-only` in about `26.810s`, and
  `EASTL/utility.h` alone rises from about `4.957s` to `10.348s` when the
  structured-binding bridge is enabled.
- Reduced the next smaller canonical blocker from `eastl_utility_simple.cpp`
  to `eastl::pair<int, int> p(4, 9);`, which now fails quickly with
  `initializer for scalar member 'pair' must have exactly one argument`.
- Added focused runtime coverage in
  `tests/cpp/internal/postive_case/ctor_init_delegating_unqualified_template_runtime.cpp`
  and fixed HIR ctor-init lowering so templated delegating constructors still
  recognize unqualified self-delegation after instantiation.
- Reduced the remaining `eastl::pair` follow-up further to a piecewise-style
  templated delegating constructor helper that still fails with
  `error: no matching constructor for delegating constructor call to
  'pair_T1_int_T2_int'`, so the current blocker is now narrower than the full
  EASTL case but not fully repaired yet.
- Promoted that piecewise-style helper into focused frontend coverage in
  `tests/cpp/internal/postive_case/ctor_init_piecewise_delegating_template_runtime.cpp`
  and fixed delegating-constructor target detection so instantiated template
  constructors preserve and match normalized source constructor family names.
- Tightened delegating-constructor overload selection so a unique non-self
  constructor with the matching arity is accepted even when templated tuple /
  index-sequence argument inference is still too imprecise for full scoring.
- Confirmed the new focused piecewise delegating-helper regression now compiles,
  and `tests/cpp/eastl/eastl_utility_simple.cpp` no longer fails with the old
  scalar-member / missing-constructor diagnostics; the remaining frontier has
  moved deeper into later canonical lowering.
- Reduced that later canonical lowering crash to a smaller HIR reproducer where
  re-entrant template-struct method lowering finalized a higher function ID
  before reserving its slot in `module_->functions`.
- Added focused HIR coverage in
  `tests/cpp/internal/hir_case/template_struct_method_param_reentrant_hir.cpp`
  and `cpp_hir_template_method_param_reentrant_lowering`, then fixed callable
  slot reservation so both bodyless and body-lowered functions reserve storage
  by `FunctionId` instead of current vector length.
- Confirmed the reduced HIR reproducer now passes under both the normal and
  ASan builds, `tests/cpp/eastl/eastl_utility_simple.cpp --dump-canonical`
  now completes in about `10.531s`, and `tests/cpp/eastl/eastl_tuple_simple.cpp`
  now completes under `--dump-canonical` as well.
- Re-ran the targeted HIR regressions and the full
  `ctest --test-dir build -j8 --output-on-failure` suite; the regression guard
  remains monotonic at 3287/3287 passing tests versus the earlier 3278/3278
  baseline, with zero new failing tests.
- Re-reduced the remaining `eastl_memory_simple.cpp` timeout to a smaller
  header-only parse frontier: `#include <EASTL/internal/memory_uses_allocator.h>`
  still times out after about `32s`, `#include <EASTL/tuple.h>` also lands in
  the same range, and the smaller
  `#include <EASTL/internal/function_detail.h>` smoke case still completes in
  about `15s`.
- Added focused timeout workflow coverage in
  `tests/cpp/eastl/eastl_memory_uses_allocator_frontier.cpp` and
  `tests/cpp/eastl/run_eastl_timeout_baseline.cmake`, then wired
  `cpp_eastl_memory_uses_allocator_timeout_baseline` into CTest so the current
  parser-debug timeout frontier is reproducible without the full memory driver.
- Re-checked `eastl_vector_simple.cpp` directly and confirmed the old
  parse-timeout recipe had gone stale: `--parse-only` now succeeds within the
  workflow timeout, while `--dump-canonical` reaches the next real sema
  cluster at `EASTL/vector.h:2066:2` (`operator_eq` / `operator_neq`) plus the
  existing `allocator.h` incompatible-return and `memory.h` `eastl::size`
  diagnostics shared with `eastl_memory_simple.cpp`.
- Rebased `cpp_eastl_vector_parse_recipe` onto the shared positive
  `run_eastl_parse_recipe.cmake` coverage, removed the obsolete
  `run_eastl_vector_parse_recipe.cmake` helper, and updated
  `tests/cpp/eastl/README.md` so the ladder records vector as a canonical/sema
  frontier instead of a parse frontier.
- Added focused frontend coverage in
  `tests/cpp/internal/postive_case/namespaced_function_param_shadow_frontend.cpp`
  and fixed sema value lookup so truly unqualified identifiers can fall back to
  their preserved source spelling before namespace-canonicalized lookup.
- Confirmed the change clears the shared `EASTL/memory.h` `eastl::size` failure
  cluster: `tests/cpp/eastl/eastl_memory_simple.cpp --dump-canonical` now
  succeeds, and `tests/cpp/eastl/eastl_vector_simple.cpp --dump-canonical`
  narrows to vector/allocator-only diagnostics.
- Added focused frontend coverage in
  `tests/cpp/internal/postive_case/qualified_member_typedef_functional_cast_frontend.cpp`
  and strengthened
  `tests/cpp/internal/postive_case/namespaced_out_of_class_method_context_frontend.cpp`
  so out-of-class methods keep the correct owner record even when another
  namespace defines the same unqualified type name.
- Fixed sema owner resolution for out-of-class methods by preferring struct
  definitions in the method's namespace context, and let owner-qualified cast
  targets inside methods defer through sema instead of rejecting
  `base_type::allocator_type(...)` immediately.
- Fixed C++ functional-cast parsing so record types, including typedef-like
  spellings that resolve to known records, take the constructor-call path even
  with a single argument instead of lowering as scalar-style casts.
- Confirmed `#include <EASTL/vector.h>` now canonicalizes, and
  `tests/cpp/eastl/eastl_vector_simple.cpp --dump-canonical` now completes.
- Re-ran the focused owner-context / functional-cast regressions plus the full
  `ctest --test-dir build -j8 --output-on-failure` suite; the regression guard
  remains monotonic at 3292/3292 passing tests versus the earlier
  3291/3291 baseline, with zero new failing tests.
- Added focused runtime coverage in
  `tests/cpp/internal/postive_case/inherited_base_method_call_runtime.cpp`
  so derived objects calling inherited zero-arg base methods stay covered.
- Fixed HIR member-call lowering to resolve inherited base methods through
  `find_struct_method_mangled(...)` instead of checking only the immediate
  record tag, which now lets calls like `tempDivisor.IsZero()` lower as real
  method calls instead of bogus field accesses.
- Fixed unary address-of lowering for concrete prvalues in codegen so
  `&<prvalue>` materializes temporary storage generically instead of failing in
  `StmtEmitter::emit_lval`, which moved the old
  `#include <EABase/eabase.h>` / `eastl_type_traits_simple.cpp` frontend
  blocker forward.
- Confirmed the reduced `#include <EABase/eabase.h>` compile now succeeds with
  `build/c4cll`, the new inherited-base runtime regression passes, and the
  focused EASTL parse recipes remain green.
- Re-ran the full `ctest --test-dir build -j8 --output-on-failure` suite after
  the new runtime regression was discovered by CMake globbing; the regression
  guard is monotonic at 3293/3293 passing tests, with zero failing tests.
- Re-ran `cmake --build build --target eastl_type_traits_simple_workflow -j8`
  and confirmed the active type-traits blocker has shifted from frontend
  failure to a later backend handoff: clang now rejects the emitted IR at
  `build/eastl_type_traits_simple/eastl_type_traits_simple.ll:316` because
  `%p.__l` has type `%\"struct.std::byte\"` while `or i32` expects an integer
  operand.
- Added focused runtime coverage in
  `tests/cpp/internal/postive_case/template_variable_trait_runtime.cpp` so
  variable-template trait adapters that read inherited dependent `::value`
  members stay covered through runtime lowering.
- Added on-demand HIR materialization for referenced variable-template globals
  instead of lowering only the unspecialized primary, which fixes the old
  `eastl::is_signed_v<int>` / `eastl::is_unsigned_v<int>` mismatch and moves
  `eastl_type_traits_simple_workflow` past exit `10`.
- Narrowed the new template-static-member fast path to the signedness /
  sameness / reference / const trait families so existing NTTP-default and
  `sizeof...(pack)` static-member paths keep their old behavior.
- Re-ran the focused scoped-enum and template-variable runtime regressions plus
  the full `ctest --test-dir build -j8 --output-on-failure` suite; the
  regression guard remains monotonic at 3295/3295 passing tests versus the
  earlier 3278/3278 baseline, with zero failing tests.
- Re-ran `cmake --build build --target eastl_type_traits_simple_workflow -j8`
  and confirmed the active type-traits blocker moved again: the c4c-built
  binary no longer exits `10`, but now exits `22`, so signedness is fixed and
  the next slice is alias-transformed trait normalization.

## Next Slice

- keep the structured-binding feature gate in place and treat vector as
  revalidated rather than active
- reduce the new `eastl_type_traits_simple_workflow` runtime mismatch to the
  smallest EASTL or internal reproducer behind
  `check_reference_transforms<int>()` returning `22`
- keep the new scoped-enum and template-variable runtime regressions green
  while tracing why alias-backed trait operands still miss `is_reference_v`
  and `is_same_v` in the c4c-built binary

## Blockers

- structured bindings themselves are still unsupported in the frontend, so the
  compiler must continue advertising `EA_COMPILER_NO_STRUCTURED_BINDING` until
  a dedicated structured-binding implementation lands
- `eastl_type_traits_simple.cpp` no longer fails in the parser/backend path,
  but its standalone workflow still diverges at runtime: the c4c-built binary
  now exits `22` while the host binary exits `0`

## Resume Notes

- use [tests/cpp/eastl/README.md](/workspaces/c4c/tests/cpp/eastl/README.md) as
  the current stage inventory
- Step 1 foundation cases already parse and mostly fail later in sema
- `eastl_utility_simple.cpp` now passes both `--parse-only` and
  `--dump-canonical`
- `eastl_tuple_simple.cpp` now also passes `--dump-canonical`, so the next
  active EASTL frontier is no longer parse-only
- `eastl_vector_simple.cpp` now also passes `--dump-canonical`, so vector is no
  longer the active EASTL frontier
- `eastl_type_traits_simple.cpp` now also passes `--dump-canonical`; the active
  frontier is the standalone workflow runtime mismatch, not the old sema or
  `std::byte` backend verifier cluster; the signedness half is fixed, but the
  alias-transformed trait half is still open
- focused scoped-enum runtime coverage now exists under
  `tests/cpp/internal/postive_case/scoped_enum_bitwise_runtime.cpp`
- focused parser coverage now exists for shadowed-name assignment dispatch
  under `tests/cpp/internal/postive_case/local_value_shadows_*`
- focused parser coverage now also exists for out-of-class constructor-template
  delegating init arguments under
  `tests/cpp/internal/postive_case/ctor_init_out_of_class_dependent_typename_index_tuple_parse.cpp`
- focused direct-init regression coverage now exists under
  `tests/cpp/internal/postive_case/local_direct_init_paren_runtime.cpp` and
  `tests/cpp/internal/postive_case/local_direct_init_single_identifier_runtime.cpp`
- focused assignment-expression lvalue regression coverage now exists under
  `tests/cpp/internal/postive_case/assignment_expr_return_lvalue_ref_runtime.cpp`
- focused concept-id `if constexpr` frontend coverage now exists under
  `tests/cpp/internal/postive_case/cpp20_if_constexpr_concept_id_frontend.cpp`
- focused inline class-template member-context frontend coverage now exists
  under
  `tests/cpp/internal/postive_case/template_inline_method_member_context_frontend.cpp`
- focused constrained concept-shorthand frontend coverage now exists under
  `tests/cpp/internal/postive_case/constrained_template_method_call_frontend.cpp`
- focused constrained member-template conversion-operator frontend coverage now
  exists under
  `tests/cpp/internal/postive_case/constrained_member_conversion_operator_frontend.cpp`
- focused ctor-init runtime coverage now also exists under
  `tests/cpp/internal/postive_case/ctor_init_member_typedef_ctor_runtime.cpp`
  and
  `tests/cpp/internal/postive_case/ctor_init_member_default_value_init_runtime.cpp`
- focused delegating-ctor runtime coverage now also exists under
  `tests/cpp/internal/postive_case/ctor_init_delegating_unqualified_template_runtime.cpp`
- focused piecewise delegating-helper frontend coverage now also exists under
  `tests/cpp/internal/postive_case/ctor_init_piecewise_delegating_template_runtime.cpp`
- focused HIR re-entrant method-lowering coverage now also exists under
  `tests/cpp/internal/hir_case/template_struct_method_param_reentrant_hir.cpp`
- the older constrained concept-shorthand scratch repro now parses and lowers,
  the member-init `first` diagnostic is gone, and both
  `eastl_utility_simple.cpp` and `eastl_tuple_simple.cpp` now complete through
  `--dump-canonical`
- `EA_COMPILER_NO_STRUCTURED_BINDING` is now predefined for C++ source
  profiles because the frontend still rejects `auto [a, b]`
- `eastl_memory_uses_allocator_frontier.cpp` now passes parse-only and
  canonical, and `eastl_memory_simple.cpp` now passes parse-only
- runtime and ABI glue remain explicitly out of scope except for temporary local
  shims already allowed by the source idea
