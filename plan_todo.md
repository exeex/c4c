# Plan Execution State

## Active Item
Operator Overloading — item 4: return to EASTL/std_vector bring-up

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
- [x] Unqualified static member call resolution in out-of-class method bodies
  - Root cause: `lower_call_expr` didn't check `struct_methods_` for unqualified callee
    names when inside a method body — only field names were resolved implicitly
  - Fix: in `lower_call_expr`, when callee NK_VAR name isn't a known global function
    and we're inside a method body, look up `method_struct_tag::callee_name` in
    `struct_methods_`; if found, resolve the call and inject implicit `this` arg
  - Free functions still take precedence (checked first via `direct_callee_fn`)
  - Test: `unqualified_static_member_call_runtime.cpp` (runtime, B::StaticAdd called as StaticAdd)

## Next Slice
- Item 4: return to EASTL/std_vector bring-up (`tests/cpp/eastl/std_vector_simple.cpp`)
  - Run the bring-up case and identify the next blocker
  - Determine whether it's operator-related or a new frontend category

## Blockers
- None

## Suite Status
- Before: 2084/2085 (1 pre-existing alignas failure)
- After: 2085/2086 (+1 new test, no regressions)
