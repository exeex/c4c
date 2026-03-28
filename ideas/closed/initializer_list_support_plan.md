# `initializer_list` Frontend Support Plan

Status: Complete
Completed: 2026-03-28

Target area: frontend semantic/lowering support for `initializer_list`-based
construction and calls  
Primary goal: implement the missing `initializer_list` functionality needed to
pass the current positive runtime probe.

## Why This Idea

One remaining positive testcase appeared to depend on actual
`initializer_list` support rather than a narrow bug fix:

- `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp`

This was tracked separately from the other template/sema regressions so the
work could land as a bounded feature slice.

## Objective

Implement enough `initializer_list` parsing, semantic modeling, and lowering to
support the positive runtime scenario without broad STL emulation work.

## Outcome

Completed in one slice:

- identified the first failing behavior as call-site lowering of
  `sum_and_check({1, 4, 7})` to `sum_and_check(0)` despite correct
  `std::initializer_list<int>` type modeling
- added focused LLVM regression coverage for backing-array materialization
- implemented hidden backing-array plus `{ptr, len}` temporary materialization
  for by-value `std::initializer_list<T>` brace-init call arguments
- taught LIR dead-code elimination to drop unreferenced `std::__...` helper
  definitions that would otherwise cause runtime link failures in this slice

## Supported Slice

The completed slice covers:

- brace-init to by-value `std::initializer_list<T>` call arguments
- backing storage materialization
- observable `begin()`, `end()`, and `size()` behavior for the acceptance case

## Validation

- `cpp_positive_sema_eastl_probe_initializer_list_runtime_cpp` now passes
- focused LLVM regression coverage documents backing-array materialization
- full-suite regression guard passed:
  before `2239/2240` passed with `1` failure
  after `2241/2241` passed with `0` failures

## Follow-On Gaps

- Full standards-complete list-initialization semantics across all contexts
  remain out of scope for this closed slice.
