# Plan Execution State

## Baseline

- 1889/1889 tests passing (2026-03-18)

## Replan Summary

This plan is now narrowed to the remaining template work that still matters for the current milestone.

Already considered done enough for this round:

- template function basics
- non-type template parameters (NTTP)
- mixed type + NTTP function templates
- delayed template instantiation in HIR
- consteval / template interaction that does not require RTTI
- template argument deduction for common function-call cases

Not in scope for now:

- RTTI-related work
- `dynamic_cast`
- any feature whose correctness depends on RTTI materialization/runtime support

## Remaining Work

The unfinished work we should treat as active is:

1. `template struct`
2. mixed nested cases between `template function` and `template struct`

Concretely, the focus is:

- template struct definition/preservation/instantiation that is reliable end-to-end
- type substitution inside template struct fields
- nested template struct forms such as `Pair<Pair<int>>`
- template struct usage inside template function bodies
- mixed nested cases such as:
  - `Pair<T>`
  - `Box<Pair<T>>`
  - `Pair<typename_id<...>>`-style deferred shapes if they do not need RTTI
- deferred resolution at HIR instantiation time rather than eager frontend-only rewriting

## What We Are Explicitly Not Tracking In This Plan

These may be revisited later, but should not block the current milestone:

- RTTI
- `dynamic_cast`
- template features that require RTTI-aware lowering or runtime metadata
- operator overloading follow-up work

## Next Milestone

The next meaningful milestone is:

Complete template struct support, including mixed nested template function/template struct cases, without expanding scope into RTTI.

## Suggested Execution Order

1. Stabilize plain `template struct` instantiation and substitution.
2. Cover nested `template struct` forms and parser/lowering edge cases such as `>>`.
3. Cover mixed nested usage inside template functions.
4. Verify deferred HIR-side instantiation still owns the final resolution path.
5. Keep RTTI-dependent scenarios excluded from both implementation and test expectations.

## Validation Targets

Add or keep coverage for:

- plain template struct field substitution
- nested template structs
- template struct with NTTP where RTTI is not involved
- template function returning or locally using template structs
- template function bodies that instantiate nested template structs

Do not spend effort in this cycle on:

- RTTI validation
- `dynamic_cast` validation

