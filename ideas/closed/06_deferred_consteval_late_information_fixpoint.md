# Deferred Consteval Late-Information Fixpoint

Status: Closed
Last Updated: 2026-04-11

## Goal

Teach c4c's compile-time engine to revisit deferred `consteval` work when
later translation-unit information unlocks results that were not computable at
first contact.

The target is not just "more `consteval` support" in isolation. The target is
whole-TU iterative convergence for compile-time programs that behave more like
mini compilers, schedulers, or rule solvers.

## Why This Idea Exists

Current c4c already supports useful deferred compile-time reduction after
deferred template instantiation. Existing internal tests such as:

- `tests/cpp/internal/postive_case/deferred_consteval_chain.cpp`
- `tests/cpp/internal/postive_case/deferred_consteval_multi.cpp`

show that the pipeline can defer some work and later converge.

But a stronger class of cases still appears to stop short of actual
convergence:

- a `consteval` query depends on a type that is incomplete at first use and
  becomes complete later in the translation unit
- a `consteval` query depends on a late explicit specialization or rule-table
  entry that appears after the first query site
- a branch, cost model, shape query, or lowering decision depends on values
  that only become available after more declarations are seen

Mainstream `g++` / `clang++` reject many of these cases immediately under
standard C++ instantiation timing rules. c4c's value proposition can be
different here, especially for compile-time workloads that resemble neural
network compiler logic.

## Main Objective

Investigate whether c4c can soundly represent these unresolved `consteval`
queries as pending work and then discharge them during later compile-time
iterations once the required semantic information becomes available.

Success may require one of two outcomes:

1. support a useful subset of these late-information cases as an intentional
   c4c extension
2. explicitly diagnose and bound the unsupported cases so the model stays
   coherent

## Candidate Cases

The exploratory file below captures current candidate patterns:

- `tests/cpp/internal/consteval_case/deferred_consteval_candidates.cpp`

Important categories from that file:

1. incomplete type later becomes complete and unlocks `sizeof(T)` inside
   `consteval`
2. late explicit specialization unlocks a pending compile-time query
3. multi-stage shape/layout/cost chains propagate newly available values
4. `if constexpr` branch selection depends on late-known compile-time facts
5. rule-table style template lookups model compile-time legalization/scheduling

## Primary Scope

- `src/frontend/sema/consteval.cpp`
- compile-time reduction scheduling and pending-work bookkeeping in HIR/frontend
- template instantiation paths that currently drop, eagerly fail, or silently
  finalize unresolved `consteval` inputs
- targeted internal regression coverage for late-information cases

## Key Questions

1. Where does c4c currently lose these cases?
2. Are unresolved queries being eagerly collapsed to "not constant", default
   values, or missing reductions?
3. What semantic events should re-queue pending `consteval` work?
4. Which late-information patterns are safe and intentional as c4c extensions?
5. Which patterns should remain rejected even in c4c to avoid unstable or
   ambiguous semantics?

## Constraints

- do not regress currently passing deferred-consteval behavior
- avoid silently materializing wrong default values for unresolved compile-time
  expressions
- keep diagnostics understandable when a query stays unresolved after the final
  fixpoint pass
- be explicit when c4c behavior intentionally diverges from standard C++

## Validation

At minimum:

- existing deferred-consteval internal tests
- focused new cases derived from
  `tests/cpp/internal/consteval_case/deferred_consteval_candidates.cpp`
- HIR dumps or compile-time stats proving pending work was revisited
- no newly failing internal `consteval` regressions

## Completion

Completed on 2026-04-11.

Implemented a bounded late-information fixpoint for deferred `consteval`
evaluation by preserving pending work until later translation-unit passes can
revisit it with newly available HIR layout information. This closure also
folds delayed `static_assert` resolution into the compile-time engine itself,
so engine-driven c4c-only convergence no longer depends on an extra
`c4cll`-side post-pass.

The shipped coverage now includes:

- incomplete-type recovery in
  `tests/cpp/internal/postive_case/deferred_consteval_incomplete_type.cpp`
- c4-only delayed-assert acceptance for
  `tests/cpp/internal/consteval_case/if_constexpr_branch_unlocks_later.cpp`
- c4-only delayed-assert acceptance for
  `tests/cpp/internal/consteval_case/multistage_shape_chain.cpp`
- HIR visibility for the recovered values in
  `tests/cpp/internal/hir_case/if_constexpr_branch_unlocks_later_hir.cpp`,
  `tests/cpp/internal/hir_case/multistage_shape_chain_hir.cpp`, and
  `tests/cpp/internal/postive_case/late_specialization_unlocks_wrapper.cpp`
- engine-time failure coverage via
  `tests/cpp/internal/negative_case/static_assert_if_constexpr_branch_unlocks_later_false.cpp`
  and
  `tests/cpp/internal/negative_case/static_assert_multistage_shape_chain_false.cpp`

## Outcome

- Deferred `consteval` queries that depend on late-known tagged-record layout
  can now converge on a later pass instead of staying permanently unresolved.
- Delayed `static_assert` checks for non-template-owned nodes now participate
  in the same compile-time-engine fixpoint as pending `consteval` work, so
  c4c-only cases can compile or fail based on the engine's converged result.
- `c4cll` no longer carries a separate post-HIR `static_assert` retry path;
  compile-time diagnostics are produced by the engine and surfaced through
  normal sema validation results.
- The targeted validation slice passed:
  `deferred_consteval_chain`, `deferred_consteval_multi`,
  `deferred_consteval_incomplete_type`,
  `cpp_c4_static_assert_if_constexpr_branch_unlocks_later`,
  `cpp_c4_static_assert_multistage_shape_chain`,
  `cpp_hir_if_constexpr_branch_unlocks_later`,
  `cpp_hir_multistage_shape_chain`,
  `cpp_hir_late_specialization_unlocks_wrapper`,
  `cpp_negative_tests_static_assert_consteval_false`,
  `cpp_negative_tests_static_assert_if_constexpr_branch_unlocks_later_false`,
  and `cpp_negative_tests_static_assert_multistage_shape_chain_false`.
- HIR validation for
  `tests/cpp/internal/postive_case/deferred_consteval_incomplete_type.cpp`
  shows `consteval get_size<T=struct TensorDesc>() = 64` with
  `1 consteval reduction (converged)`.
- HIR validation for the new c4-only slices shows
  `consteval choose_tile<T=struct TileShape>() = 128` and
  `consteval vec_cost<T=struct F32x8>() = 24`, confirming that the engine
  revisits pending work until the late information becomes usable.

## Leftover Issues

- The late-validation mechanism is in place, but not every exploratory
  late-information pattern converges yet.
- `tests/cpp/internal/consteval_case/baseline_nested_template_consteval.cpp`
  now reaches delayed `_Static_assert` evaluation, but still computes the
  wrong result (`bar<5>()` still behaves like `2` instead of `3`).
- `tests/cpp/internal/consteval_case/incomplete_type_unlocks_later.cpp`
  in its original class-template default-NTTP form still settles on the wrong
  value and fails `_Static_assert condition is false`.
- The more ambitious candidate shapes in
  `tests/cpp/internal/consteval_case/deferred_consteval_candidates.cpp`,
  especially the original default-NTTP / rule-table variants, remain bounded
  rather than fully supported by this closure.
- A clean full-suite rebuild during this work still observed one unrelated
  failure:
  `cpp_positive_sema_c_style_cast_base_ref_qualified_method_cpp`, caused by a
  backend/Clang IR type mismatch involving `%struct.Derived` and
  `%struct.Base`. This is outside the deferred-consteval late-information
  fixpoint scope.

## Non-Goals

- not a full redesign of all template semantics
- not a promise that every "late information" C++ pattern becomes legal
- not backend or codegen optimization work
