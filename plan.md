# Implementation Plan

## Goal

Finish the remaining template work by focusing only on:

- `template struct`
- mixed nested cases between `template function` and `template struct`

For this planning round, RTTI-related work is intentionally out of scope.

## Scope

### In Scope

- template struct definition handling
- template struct parameter substitution
- template struct instantiation in normal and deferred paths
- nested template struct types
- mixed nested template function/template struct usage
- HIR ownership of deferred instantiation for these cases

### Out Of Scope

- RTTI
- `dynamic_cast`
- RTTI-dependent lowering/materialization/runtime behavior

## Current Assessment

The broad compile-time architecture is good enough for this milestone:

- sema remains conservative
- HIR owns the delayed template/consteval reduction path
- template function support is far enough along that the remaining risk is now concentrated in template structs and mixed nesting

So the plan should stop expanding feature surface and instead close the remaining template-struct gaps.

## Required Behavior

By the end of this milestone, we want:

1. `template<typename T> struct Box { T value; };` to work reliably.
2. nested forms like `Box<Box<int>>` to parse and lower correctly.
3. template functions to use template structs in deferred contexts, for example:

```cpp
template<typename T>
Box<T> make_box(T x) {
    Box<T> out;
    out.value = x;
    return out;
}
```

4. mixed nested forms to survive until HIR instantiation time, for example:

```cpp
template<typename T>
Box<Pair<T>> wrap_pair(T a, T b);
```

5. none of the above to depend on RTTI support.

## Implementation Sequence

### Phase A: stabilize plain template struct support

Goals:

- make non-nested template structs reliable end-to-end

Tasks:

- verify template struct definitions preserve template parameter bindings
- ensure field types substitute correctly during instantiation
- ensure instantiated struct layouts are represented consistently
- keep instantiation ownership in the existing deferred/HIR path where applicable

Exit criteria:

- plain `template struct` cases compile and lower consistently
- field substitution is correct in emitted/instantiated forms

### Phase B: nested template struct forms

Goals:

- support nested template struct types without parser/lowering ambiguity

Tasks:

- validate nested type parsing such as `Pair<Pair<int>>`
- ensure canonical type construction works for nested template args
- ensure lowering does not collapse or mis-resolve nested template struct types

Exit criteria:

- nested template struct cases work in declarations, locals, params, and returns

### Phase C: mixed template function + template struct nesting

Goals:

- make deferred template function instantiation cooperate with template struct instantiation

Tasks:

- support `Pair<T>` and `Box<Pair<T>>` inside template function bodies
- support return types / local variables / call arguments involving template structs
- ensure substitution order is correct when both function-template params and struct-template params participate
- keep resolution deferred until enough bindings are known

Exit criteria:

- mixed nested function/struct cases work without eager frontend-only hacks

### Phase D: targeted validation and cleanup

Goals:

- lock in the milestone without scope creep

Tasks:

- add regression tests for plain, nested, and mixed nested template-struct cases
- confirm no RTTI-dependent scenario is accidentally required by the tests
- remove stale planning notes that imply RTTI work is part of this milestone

Exit criteria:

- tests cover the intended surface
- plan/docs/code comments all reflect the narrowed scope

## Checklist

### Template Struct Core

- [ ] Preserve template struct parameter bindings through lowering
- [ ] Substitute template params in struct fields correctly
- [ ] Instantiate concrete struct forms deterministically
- [ ] Keep canonical naming/identity stable for instantiated template structs

### Nested Template Structs

- [ ] Parse nested `>>` forms correctly
- [ ] Lower nested template struct types correctly
- [ ] Handle nested template structs in locals/params/returns

### Mixed Function/Struct Nesting

- [ ] Support template structs inside template function bodies
- [ ] Support template struct return types in template functions
- [ ] Support nested mixed forms such as `Box<Pair<T>>`
- [ ] Ensure deferred instantiation resolves these cases at HIR time

### Explicit Non-Goals

- [ ] Do not add RTTI requirements to this milestone
- [ ] Do not work on `dynamic_cast` in this milestone

## Regression Targets

- [ ] Existing template function tests still pass
- [ ] Existing consteval-template tests that do not depend on RTTI still pass
- [ ] Add/keep a plain template struct test
- [ ] Add/keep a nested template struct test
- [ ] Add/keep a mixed template function + template struct test

## Milestone Definition

This milestone is complete when:

- plain `template struct` works
- nested `template struct` works
- mixed nested `template function` + `template struct` cases work
- no RTTI work was required to get there
