# Plan Execution State

## Baseline

- 1891/1891 tests passing (2026-03-18)

## Status: MILESTONE COMPLETE

All phases of the template struct plan are complete.

## Completed Work

### Phase A: stabilize plain template struct support — DONE
- template struct definitions preserve template parameter bindings ✓
- field types substitute correctly during instantiation ✓
- instantiated struct layouts represented consistently ✓
- plain template struct cases compile and lower consistently ✓

### Phase B: nested template struct forms — DONE
- nested type parsing (Pair<Pair<int>>, Box<Pair<int>>) works ✓
- triple-nested (Box<Box<Box<int>>>) works ✓
- canonical type construction works for nested template args ✓
- nested template struct cases work in declarations, locals, params, returns ✓

### Phase C: mixed template function + template struct nesting — DONE
- Pair<T> and Box<Pair<T>> inside template function bodies works ✓
- return types / local variables / call arguments involving template structs ✓
- substitution order is correct ✓
- deferred resolution works at HIR time ✓

### Phase C fix: template struct method with template struct return type
- **Root cause**: Parser method cloning during template struct instantiation only substituted simple TB_TYPEDEF types, not pending template struct types (tpl_struct_origin)
- **Symptom**: Method like `Pair<T> as_pair()` on `Container<T>` had unresolved `Pair_T_T` after instantiating `Container<int>`
- **Fix**: Store template bindings on instantiated struct node; HIR extracts and passes them to lower_struct_method

### Phase D: targeted validation and cleanup — DONE
- [x] add regression tests for plain, nested, and mixed nested template-struct cases
- [x] add test for template struct method returning template struct
- [x] add test for triple-nested, deferred double-nested, non-template struct with template fields, pointers
- [x] confirm no RTTI-dependent scenario is accidentally required by the tests
- [x] no stale planning notes that imply RTTI work in source or tests

## Checklist — All Complete

### Template Struct Core
- [x] Preserve template struct parameter bindings through lowering
- [x] Substitute template params in struct fields correctly
- [x] Instantiate concrete struct forms deterministically
- [x] Keep canonical naming/identity stable for instantiated template structs

### Nested Template Structs
- [x] Parse nested `>>` forms correctly
- [x] Lower nested template struct types correctly
- [x] Handle nested template structs in locals/params/returns

### Mixed Function/Struct Nesting
- [x] Support template structs inside template function bodies
- [x] Support template struct return types in template functions
- [x] Support nested mixed forms such as `Box<Pair<T>>`
- [x] Ensure deferred instantiation resolves these cases at HIR time

### Explicit Non-Goals
- [x] Do not add RTTI requirements to this milestone
- [x] Do not work on `dynamic_cast` in this milestone

## Regression Targets — All Complete
- [x] Existing template function tests still pass
- [x] Existing consteval-template tests still pass
- [x] Plain template struct test
- [x] Nested template struct test
- [x] Mixed template function + template struct test

## Test Coverage Summary

7 template struct test files:
- template_struct.cpp — basic field substitution with int/long
- template_struct_nttp.cpp — NTTP with Array<T,N>, IntBuf<N>
- template_struct_keyword.cpp — struct keyword syntax
- template_struct_method.cpp — member functions on template structs
- template_struct_method_tpl_return.cpp — methods returning template structs (NEW)
- template_struct_nested.cpp — nested + mixed deferred (Box<Pair<T>>)
- template_struct_advanced.cpp — triple-nested, pointers, non-template struct fields (NEW)
