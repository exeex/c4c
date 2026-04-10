# Deferred Consteval Late-Information Fixpoint Todo

Status: Active
Source Idea: ideas/open/06_deferred_consteval_late_information_fixpoint.md
Source Plan: plan.md

- [x] Read the current deferred `consteval` scheduling path and identify where
      pending work is finalized or lost.
- [x] Define the minimal pending-state and retry semantics for late
      translation-unit information.
- [x] Implement bounded fixpoint revisit behavior for deferred `consteval`
      work.
- [x] Add or refine targeted tests for supported and explicitly unsupported
      late-information cases.
- [x] Run focused validation for deferred-consteval behavior and update this
      file with results.

## Current Slice

- Active item: none recorded; the incomplete-type deferred `consteval`
  late-layout slice is implemented.
- Next intended slice: extend late-information coverage beyond record-layout
  recovery if the active plan remains open, especially explicit specialization
  and multi-stage propagation candidates.

## Notes

- Loss point identified: HIR deferred `consteval` evaluation had no access to
  late-known record layout, so `sizeof(T)` / `alignof(T)` on a tagged record
  stayed pending even after the translation unit had completed the type.
- Implemented rule: when the compile-time engine evaluates a pending
  `consteval`, it now passes HIR struct layout metadata into the constant
  evaluator so tagged record queries can resolve on a later pass.
- Added regression:
  `tests/cpp/internal/postive_case/deferred_consteval_incomplete_type.cpp`
  plus `cpp_hir_deferred_consteval_incomplete_type`.

## Validation

- Targeted HIR dump:
  `./build/c4cll --dump-hir tests/cpp/internal/postive_case/deferred_consteval_incomplete_type.cpp`
  now shows `consteval get_size<T=struct TensorDesc>() = 64` and `1 consteval reduction (converged)`.
- Targeted suite:
  `ctest --test-dir build -R 'deferred_consteval_(chain|multi|incomplete_type)' --output-on-failure`
  passed `6/6`.
- Full-suite baseline before change:
  `test_before.log` = `3363 passed / 0 failed / 3363 total`.
- Full-suite after clean rebuild:
  `test_after.log` = `3364 passed / 1 failed / 3365 total`.
- Remaining blocker from the clean full-suite run:
  `cpp_positive_sema_c_style_cast_base_ref_qualified_method_cpp`
  failed with a backend/clang IR type mismatch that appears unrelated to the
  deferred-`consteval` slice:
  `%struct.Derived = type { %struct.Base }` stored where `%struct.Base = type {}`
  was expected.
