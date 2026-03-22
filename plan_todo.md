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
- [x] EASTL bring-up slice 4: typename, typedef/struct redefinition, C++ struct body features
  - `typename` dependent type: after consuming `typename` keyword, parser now consumes
    the FULL dependent type expression (Ident::Ident<...>::Ident) including nested
    template args and `::type` suffixes (e.g. `typename eastl::conditional<...>::type`)
  - Template specialization inner typedef redefinition: C++ mode now silently overwrites
    conflicting typedefs instead of throwing (template specializations legitimately
    redefine inner typedefs with different types)
  - Template specialization inner struct redefinition: C++ mode uses shadow tags instead
    of throwing for top-level struct redefinition (e.g. `no_type_helper` in both primary
    and specialization)
  - Template specialization arg parsing: wrapped in try-catch for graceful handling of
    unparseable args like `T U::*` (member pointer types); falls back to balanced skip
  - Unresolved qualified names: `parse_base_type()` now treats unresolved `Ident::Ident`
    as TB_TYPEDEF in C++ mode (for unknown namespace/dependent types)
  - `constexpr`/`consteval` in `parse_base_type()`: consumed like `static`/`inline`
    (was missing, causing `static constexpr bool` to fail in struct bodies)
  - Static data member initializer: `= expr` skipping in struct body fields (handles
    `static constexpr bool value = complex_expression;`)
  - C++ access specifiers: `public:`, `private:`, `protected:` consumed in struct bodies
  - C++ `friend` declarations: `friend Type;` skipped in struct bodies
  - Statement disambiguation: `Type::Method(args)` vs declaration — uses full qualified
    name resolution via `peek_qualified_name` (was only checking first 2 segments)
  - EASTL errors reduced from ~21 to 9 (mostly cascading from a few remaining blockers)
  - Test: `eastl_slice4_typename_and_specialization_parse.cpp` (parse-only)
- [x] C++ member-pointer declarator parsing (`T C::*`)
  - Root cause: `parse_base_type()` consumed the class name in declarations like
    `R C::*member` as if it were another typedef/base type, so parser failed before
    declarator handling
  - Fix: stop base-type parsing when identifier is followed by `::*`, and teach
    `parse_declarator()` to consume `C::*` / `ns::C::*` as a pointer-like declarator
  - This is enough for parser bring-up of EASTL cases like
    `invoke_impl(R C::*func, T&& obj, ...)`
  - Test: `member_pointer_param_parse.cpp` (parse-only)
- [x] C++ trailing return type parsing (`auto f(...) -> type`)
  - Root cause: after parameter list / trailing cv / exception-spec parsing, function
    and method paths never consumed `-> return_type`, leaving parser state corrupted or
    silently producing malformed AST
  - Fix: in top-level function parsing and struct-body method/operator parsing, detect
    `->` after `skip_exception_spec()` and parse a replacement return type via
    `parse_type_name()`
  - Validated both plain trailing return types and dependent forms like
    `decltype(f())`
  - Test: `trailing_return_type_runtime.cpp` (runtime)

## Next Slice
- EASTL bring-up slice 5: remaining blockers (~9 errors):
  - iterator.h:395: cascading error from complex template member functions with
    `enable_if<...>::type` return types and `EASTL_REMOVE_AT_2024_SEPT` deprecated attributes
  - functional_base.h:197: template member functions with own template params (`template<typename A, typename B>`)
  - Need to handle: template member function params in struct body, especially when
    combined with `enable_if<...>::type` return types and deprecated attribute macros

## Blockers
- None critical

## Suite Status
- Before: 2097/2097
- After: 2098/2098 (+1 new test, no regressions)
