# `initializer_list` Frontend Support Plan

Target area: frontend semantic/lowering support for `initializer_list`-based
construction and calls  
Primary goal: implement the missing `initializer_list` functionality needed to
pass the current positive runtime probe.

## Why This Idea

One remaining positive testcase appears to depend on actual
`initializer_list` support rather than a narrow bug fix:

- `223 - cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`

This should be tracked separately from the other template/sema regressions so
we can treat it as a feature slice with clear boundaries, validation, and
follow-up work if the support surface is larger than expected.

## Objective

Implement enough `initializer_list` parsing, semantic modeling, and lowering to
support the currently failing positive runtime scenario without broad STL
emulation work.

## Suspected Capability Gaps

Likely missing pieces may include:
- recognizing `std::initializer_list<T>` or equivalent frontend type modeling
- brace-init handling that selects initializer-list constructors or call paths
- materializing backing array/storage lifetime for initializer-list objects
- passing initializer-list size/data pairs through sema/lowering/codegen

The first step should confirm which of these is actually missing for the
failing test.

## Execution Plan

### 1. Characterize the current failure

- capture the exact parser/sema/lowering/runtime failure for
  `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`
- identify the first unsupported language step rather than only the final
  runtime symptom

### 2. Define the minimum supported model

Support only the subset needed by the testcase first:
- brace-init to `initializer_list`
- constructor/overload selection that consumes initializer lists
- backing storage materialization and observable iteration semantics

Explicit non-goal for the first slice:
- full standards-complete list-initialization semantics across all contexts

### 3. Implement frontend-to-runtime path

Potential surfaces:
- parser handling for brace-init expressions
- sema/type construction for `initializer_list`
- lowering/storage materialization
- codegen/runtime ABI for element pointer plus element count

### 4. Add focused regression coverage

- keep the existing positive runtime case as the top-level acceptance test
- add smaller focused frontend/HIR coverage where possible for list
  construction and consumption

## Guardrails

- Do not hide missing `initializer_list` support behind testcase-specific
  special cases.
- Keep the first implementation intentionally narrow and behavior-driven.
- If full list-initialization support proves larger, land the smallest useful
  slice and record follow-on gaps in a separate idea update.

## Success Criteria

- `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp` passes.
- The implementation is generic enough to support the tested
  `initializer_list` flow without EASTL-only hardcoding.
- Focused regression coverage documents the supported subset.
