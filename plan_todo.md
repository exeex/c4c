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
- [x] EASTL bring-up slice 1: parser fixes for header parsing
  - Bump __GNUC__ from 4.2 to 10.0 to avoid EASTL nullptr polyfill (member-ptr syntax)
  - Variadic template parameter packs (`typename...`, `class...`, `bool...`, `size_t...`)
  - Non-type template parameters with typedef types (`size_t N`, `T v`)
  - Immediate injection of type params for NTTP resolution (`template<typename T, T v>`)
  - Free operator function parsing (operator==, operator!= at namespace/global scope)
  - `delete`/`delete[]` expression parsing (parsed as no-op)
  - Skip sema validation for template-parameterized global declarations
  - Extra template args graceful skip (variadic pack expansion in template arg lists)
  - EASTL errors reduced from 21 to 11
  - Test: `template_variadic_and_nttp_parse.cpp` (runtime)
- [x] EASTL bring-up slice 2: template struct specialization parsing
  - Explicit specialization (`template<> struct is_void<void>`) and partial specialization
    (`template<typename T> struct remove_const<const T>`) now parsed correctly
  - Lookahead in `parse_struct_or_union()` detects `<args>` followed by `{` or `:`
  - Specialization struct gets mangled tag name (`__spec_N`) to avoid primary template collision
  - `last_struct_was_specialization_` flag prevents re-registration in `template_struct_defs_`
  - `>>` (GreaterGreater) handling in inheritance clause skipping and specialization arg consumption
  - NTTP default value `is_expr_continuation` heuristic prevents premature `>` close when
    followed by binary operators (`||`, `&&`, `::`, etc.)
  - `expect_template_close()` replaces `expect(Greater)` at template param list close
  - EASTL errors reduced from 11 to ~20 (deeper in headers — new territory)
  - Test: `template_struct_specialization_parse.cpp` (parse-only)
- [x] EASTL bring-up slice 3: _Pragma, [[attr]], typename, template arg skipping
  - `_Pragma(MACRO_ARG)`: preprocessor now expands macro arguments before processing
  - `[[deprecated]]` / `[[...]]`: C++11 attribute syntax skipped in struct body parsing
  - `typename` keyword: recognized in `is_type_start()` and `parse_base_type()` for
    dependent type expressions (`typename Container::value_type`)
  - Unresolved template arguments: `parse_base_type()` skips `<...>` for unresolved
    typedef names in C++ mode (e.g. `reverse_iterator<Iterator1>` in template context)
  - `skip_attributes()`: now also handles `[[...]]` C++11 attribute syntax
  - EASTL errors reduced from ~20 to ~21 (new errors from deeper parsing, different set)
  - Test: `pragma_operator_and_cpp11_attrs_parse.cpp` (runtime)

## Next Slice
- EASTL bring-up slice 4: remaining blockers (~21 errors):
  - type_transformations.h: `sizeof(T)` in NTTP template args, struct redefinition
  - type_compound.h: `::` in template args
  - iterator.h: `typename`-dependent type issues in struct method params (`&&` rvalue refs)
  - copy_help.h/fill_help.h: `enable_if<...>` in return types of free functions
  - functional_base.h: `::` in function params
  - Need to fix NTTP expression parsing to handle sizeof(), and handle `enable_if<>` return type patterns

## Blockers
- None critical

## Suite Status
- Before: 2095/2095
- After: 2096/2096 (+1 new test, no regressions)
