# Plan Execution State

## Baseline

- 1891/1891 tests passing (2026-03-18)

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

### Phase A-C fix: template struct method with template struct return type
- **Root cause**: Parser method cloning during template struct instantiation only substituted simple TB_TYPEDEF types, not pending template struct types (tpl_struct_origin)
- **Symptom**: Method like `Pair<T> as_pair()` on `Container<T>` had unresolved `Pair_T_T` after instantiating `Container<int>`
- **Fix**:
  1. Parser stores template bindings (n_template_args, template_arg_types, etc.) on instantiated struct node
  2. HIR collect_struct_def extracts bindings and passes them to lower_struct_method
  3. lower_struct_method resolves tpl_struct_origin in return type and param types
  4. Method body locals also resolve via ctx.tpl_bindings

## Remaining Work

### Phase D: targeted validation and cleanup
- [x] add regression tests for plain, nested, and mixed nested template-struct cases
- [x] add test for template struct method returning template struct
- [x] add test for triple-nested, deferred double-nested, non-template struct with template fields, pointers
- [ ] confirm no RTTI-dependent scenario is accidentally required by the tests
- [ ] remove stale planning notes that imply RTTI work is part of this milestone

## Next Intended Slice

Phase D cleanup: verify no RTTI dependency in tests, clean up stale notes.

## Validation Targets

- [x] plain template struct field substitution
- [x] nested template structs
- [x] template struct with NTTP
- [x] template function returning or locally using template structs
- [x] template function bodies that instantiate nested template structs
- [x] template struct methods returning template structs
- [x] triple-nested template structs
- [x] non-template struct with template struct fields
- [x] pointers to template structs
