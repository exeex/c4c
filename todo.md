# EASTL Container Bring-Up Todo

Status: Active
Source Idea: ideas/open/47_eastl_container_bringup_plan.md
Source Plan: plan.md

## Active Item

- Step 2: switch to the smaller active canonical frontier in
  `tests/cpp/eastl/eastl_utility_simple.cpp` before returning to
  `eastl_tuple_simple.cpp`.
- Iteration target: finish reducing the remaining `eastl::pair` piecewise /
  delegating-constructor helper failure behind
  `error: initializer for scalar member 'pair' must have exactly one argument`.
- This slice is specifically targeting the next bounded reproducer after
  confirming that `eastl_tuple_simple.cpp` still times out later in canonical
  but is no longer the smallest active case.

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

## Next Slice

- reduce the new `eastl_tuple_simple.cpp` deeper canonical timeout path to the
  next smallest internal reproducer or bounded libstdc++ / tuple checkpoint
- continue from the smaller `eastl_utility_simple.cpp` `pair` piecewise helper
  reproducer before returning to the deeper `eastl_tuple_simple.cpp` timeout
- keep `eastl_memory_simple.cpp` parked for now: after this tuple fix it still
  times out under both `--parse-only` and `--dump-canonical`, so it has not
  become the smaller frontier

## Blockers

- `eastl_tuple_simple.cpp` no longer stops on the old member-init semantic
  failure, but the next canonical frontier is now a deeper timeout with no new
  terminal diagnostic inside 30s, so it still needs a fresh reduction
- `eastl_utility_simple.cpp` is now the smaller active canonical frontier:
  direct templated self-delegation is fixed, but the remaining piecewise helper
  path still misses the internal delegating constructor overload
- `eastl_memory_simple.cpp` still times out under both parse-only and
  canonical/sema pressure, though the trace reaches much later tuple/ranges
  work than before
- `eastl_vector_simple.cpp` also times out deeper in the stack, so it is not
  currently the smallest useful frontier

## Resume Notes

- use [tests/cpp/eastl/README.md](/workspaces/c4c/tests/cpp/eastl/README.md) as
  the current stage inventory
- Step 1 foundation cases already parse and mostly fail later in sema
- `eastl_utility_simple.cpp` parse-only now succeeds and is ready for
  canonical/sema follow-up if it becomes the smaller frontier
- `eastl_utility_simple.cpp` has now become that smaller frontier; the active
  bounded failure is the `eastl::pair` piecewise delegating-helper path
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
- the older constrained concept-shorthand scratch repro now parses and lowers,
  the member-init `first` diagnostic is gone, `eastl_tuple_simple.cpp`
  `--parse-only` succeeds again, and the remaining tuple frontier is now a
  deeper canonical timeout
- runtime and ABI glue remain explicitly out of scope except for temporary local
  shims already allowed by the source idea
