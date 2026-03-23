# Plan Execution State

## Active Item
EASTL bring-up slice 7d: qualified declarator template args, expression template disambiguation

## Completed
- [x] Template conversion operator parsing in class body (template<class T> operator T*())
- [x] Implicit member lookup in out-of-class method bodies
- [x] Unqualified static member call resolution in out-of-class method bodies
- [x] Default parameter value skipping in parser
- [x] noexcept/throw() exception specification skipping in struct body methods
- [x] EASTL bring-up slice 1: parser fixes for header parsing
- [x] EASTL bring-up slice 2: template struct specialization parsing
- [x] EASTL bring-up slice 3: _Pragma, [[attr]], typename, template arg skipping
- [x] EASTL bring-up slice 4: typename, typedef/struct redefinition, C++ struct body features
- [x] C++ member-pointer declarator parsing (`T C::*`)
- [x] C++ trailing return type parsing (`auto f(...) -> type`)
- [x] Struct-body template members with `constexpr` / `consteval` prefixes
- [x] Parameter-pack declarators and call-site pack expansions
- [x] EASTL bring-up slice 5: explicit keyword, decltype, ptr-to-member-fn, constexpr init
- [x] EASTL bring-up slice 6: template defaults, ref-qualifiers, using aliases, sizeof...
  - `>>` in struct-body template default values: skip loops now split `>>` tokens
  - Ref-qualified methods (`&`/`&&`): consumed in regular and operator method paths
  - `using Name = Type;` in function body and struct body: registered as typedef
  - `sizeof...(pack)`: returns 0 placeholder
  - `constexpr`/`consteval` before constructor detection: peek past specifiers
  - `static_assert` in struct body: explicit skip handler
  - `static_assert` with template-dependent condition: silently skip in C++ mode
  - `(*const& ptr)` function pointer declarator: handle ref after qualifiers
  - Initializer `<>` tracking: protects commas in template expressions
  - Duplicate field names in C++ mode: silently allowed
  - EASTL: all original blockers resolved; 21 new errors in deeper headers
  - Test: `eastl_slice6_template_defaults_and_refqual.cpp` (runtime)
- [x] EASTL bring-up slice 7a: piecewise ctor pack-expansion / alias-template crash guard
  - Constructor initializer pack expansions `expr...` now parse without corrupting following methods
  - Added parse regression for EASTL-style `pair(piecewise_construct_t, tuple..., index_sequence...)`
  - Alias-template direct-init no longer crashes in template-struct pattern selection
  - Test: `eastl_slice7_piecewise_ctor_parse.cpp` (parse)
- [x] `operator new/delete/new[]/delete[]` declarator and explicit operator-call parsing
  - `new` is now tokenized as a C++ keyword
  - global and class-scope `operator new/delete/new[]/delete[]` declarations parse
  - out-of-class `S::operator new(...)` / `S::operator delete[](...)` definitions parse
  - explicit calls like `::operator new(...)` / `::operator new[](...)` lower to canonical operator symbols
  - Test: `operator_new_delete_overload_parse.cpp` (parse)
- [x] `new`/`delete` expression support (parse + HIR + codegen)
  - `new T` → `operator_new(sizeof(T))` cast to `T*`
  - `new T[n]` → `operator_new_array(sizeof(T) * n)` cast to `T*`
  - `::new (p) T(args)` → inlined placement (use pointer directly) + constructor call
  - `delete p` → `operator_delete(p cast to void*)`
  - `delete[] p` → `operator_delete_array(p cast to void*)`
  - Class-specific operator lookup: `StructTag::operator_new/delete` checked before global
  - Constructor call emitted after allocation when ctor args present
  - All 6 target tests now green (3 parse, 3 runtime side-effect tests)
- [x] EASTL bring-up slice 7b: template using alias, forward decl, multi-arg placement new
  - Template using alias `<args>` consumption: resolved typedefs with `<...>` after them now skip the template arguments (fixes `index_sequence<I1...>` in function params)
  - Template struct forward declarations: `template <typename T> class X;` now registered in `template_struct_defs_` (C++ mode only)
  - Multi-arg placement new: `::new(p, flags, 0, name, 0) char[n]` now parses correctly
  - Multi-arg placement new HIR: all placement args forwarded to operator_new call
  - Previous EASTL blockers resolved: allocator.h, utility.h, tuple_fwd_decls.h
  - Test: `eastl_slice7b_template_using_alias_parse.cpp` (parse)
- [x] EASTL bring-up slice 7c: struct body recovery, ctor detection, lambda skip
  - Struct body error recovery: try-catch around each member in the struct body loop
    prevents a single unparseable member from aborting the entire struct (C++ mode only)
  - Constructor/destructor detection for partial specializations: `is_ctor_name()` now
    checks both mangled struct tag and original `template_origin_name`
  - Injected-class-name for specializations: the original template name (e.g. `TupleImpl`)
    is added to `typedefs_` and `typedef_types_` so methods/params can reference it
  - Lambda expression skipping: `[captures](params) -> ret { body }` detected in
    `parse_primary()` and replaced with `0` placeholder; full balanced bracket/paren/brace
    skip handles mutable/constexpr/noexcept/trailing-return
  - EASTL error count: 13 errors → now reaching deep into vector.h (20+ lines)
  - Remaining EASTL blockers: `eastl::forward` with `.` member access, `...` variadic
    in free function templates, `::` qualified types in out-of-namespace contexts
  - Test: `eastl_slice7c_struct_body_recovery.cpp` (parse)
- [x] EASTL bring-up slice 7d: qualified declarator template args, expression template disambiguation
  - parse_declarator: skip balanced `<...>` template args in qualified names when followed
    by `::` — enables `vector<T, Allocator>::set_capacity` out-of-class method definitions
  - Empty template args `<>` followed by `{` (brace-init): accepted in expression context
    — fixes `eastl::less<>{}` pattern
  - Function type `R(Args...)` in template args: `parse_base_type()` template arg parsing
    now uses `parse_base_type()` directly (instead of `parse_type_name()` which would call
    `parse_declarator` and misinterpret the function params), then skips `(...)` suffix
  - Variable template expressions: `is_valid_after_template()` accepts expression
    terminators (`)`, `;`, `,`, binary ops) after template close — fixes
    `is_signed_v<IntSourceType>` in parens
  - C-style cast vs parenthesized expression disambiguation: when `(ns::value)` is followed
    by `)`, `;`, `,`, or other expression terminators, treat as paren expression instead
    of cast (fixes `((eastl::is_signed_v<T>))`)
  - EASTL error count: 21 → 21 but composition changed significantly:
    - Fixed: algorithm.h:2434,4120,4134, function.h:132, vector.h:104
    - Remaining: function_detail.h:237,699, tuple.h:541,566,943,
      memory_uses_allocator.h:97, memory.h:1140,1244, vector.h:905+ (cascading)
  - Test: `eastl_slice7d_qualified_declarator_parse.cpp` (parse)

## Next Slice
- EASTL bring-up slice 7e: remaining errors to investigate:
  - `function_detail.h:237` — `.` member access after call result
    (`GetFunctorPtrRef(storage) = func`) — probably inside struct body recovery scope
  - `function_detail.h:699` — `...` in expression context (pack expansion in call args?)
  - `tuple.h:541,566` — `TupleEqual<I-1>()(t1,t2)` — temporary-then-call pattern
    works in isolation but fails in EASTL context (cascading from earlier error?)
  - `tuple.h:943` — `decltype(auto)` or template arg with `<` in apply_impl
  - `memory_uses_allocator.h:97` — `.` member access in template body
  - `memory.h:1140,1244` — template param and `::` in template arg context
  - `vector.h:905+` — cascading from earlier errors in headers; works in isolation

## Exposed Failing Tests
- (none — 2117/2117 all passing)

## Blockers
- None for current slice (complete)
- vector.h errors appear to be cascading from earlier header errors (function_detail.h, tuple.h)

## Suite Status
- Before: 2116/2116
- After: 2117/2117 (all passing, 0 regressions, +1 new test)
