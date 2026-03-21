# Plan Execution State

## Active Item
Operator Overloading — item 2: implicit member lookup in out-of-class method bodies

## Completed
- [x] Template conversion operator parsing in class body (template<class T> operator T*())
  - Injected template type params as typedefs during struct body parsing
  - RAII guard cleans up injected names on scope exit
  - Test: `template_conversion_operator_parse.cpp` (parse-only)
- [x] Implicit member lookup in out-of-class method bodies
  - Root cause: sema validator rejected unqualified field names (e.g. `mPart0`) because
    `lookup_symbol` didn't check struct fields when inside a qualified method body
  - Fix: added `struct_field_names_` map and `current_method_struct_tag_` to sema validator;
    NK_VAR lookup now falls back to struct field check when inside a method body
  - HIR already had the implicit `this->field` rewrite (lower_expr NK_VAR path)
  - Test: `operator_implicit_member_runtime.cpp` (runtime, validates Point::sum/scale)

## Next Slice
- Item 3: unqualified static member call resolution/lowering in out-of-class methods
  - e.g. `B::OperatorPlus(...)` called as `OperatorPlus(...)` inside a B method body

## Blockers
- None

## Suite Status
- Before: 2084/2085 (1 pre-existing alignas failure)
- After: 2084/2085 (+1 new test, no regressions)
