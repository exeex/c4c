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
- [x] Default parameter value skipping in parser
  - Root cause: `parse_param()` didn't handle `= expr` after parameter type/name;
    EASTL allocator.h and many other headers use default parameter values
  - Fix: after `skip_attributes()` in `parse_param()`, check for `TokenKind::Assign`;
    if found, skip balanced tokens (parens, brackets, braces) until `,` or `)` at depth 0
  - Test: `default_param_value_parse.cpp` (parse-only, validates free fn/method/ctor defaults)
  - Note: values are SKIPPED, not stored — runtime default arg substitution not yet implemented
- [x] noexcept/throw() exception specification skipping in struct body methods
  - Root cause: after `)` in constructor, destructor, operator, and regular method
    declarations inside struct bodies, `noexcept`/`noexcept(expr)`/`throw()` was not
    consumed — left parser in bad state, corrupting subsequent declarations
  - Fix: added `skip_exception_spec()` helper to Parser; called after `)` in all 4
    struct body method paths (ctor, dtor, operator, regular method) and 3 out-of-class
    paths (qualified operator, qualified ctor, regular qualified method)
  - Test: `noexcept_method_parse.cpp` (runtime, validates noexcept on ctor/dtor/method)
  - Resolved exception.h:65,67 blocker (copy/move ctor with noexcept)

## Next Slice
- EASTL bring-up: remaining blockers:
  1. `nullptr.h:36` — member pointer conversion operator `operator T C::*()` (niche syntax)
  2. `allocator.h:294` — `operator delete[]` expression parsing
  3. `type_traits.h` — variadic template parameters (`...` in template param lists)
  - Most impactful next: variadic template parameter packs (used pervasively in type_traits)
  - Or: fix member pointer syntax in nullptr.h (blocks EABase)

## Blockers
- None

## Suite Status
- Before: 2085/2088 (3 pre-existing failures: alignas, template_struct_advanced, template_struct_nested)
- After: 2085/2088 (+1 new test, no regressions)
