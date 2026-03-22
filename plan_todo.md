# Plan Execution State

## Active Item
EASTL bring-up slice 7b: deeper header parsing fixes

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

## Next Slice
- EASTL bring-up slice 7c: deeper header errors now led by:
  - `EASTL/internal/function_detail.h` (5 errors): `.` member access, `::` qualified types
  - `EASTL/internal/function.h` (2 errors): `::` qualified types, `...` ellipsis
  - `EASTL/algorithm.h` (14 errors): `[` lambda expressions, `,` in complex expressions

## Exposed Failing Tests
- (none — 2115/2115 all passing)

## Blockers
- None for current slice (complete)
- EASTL deeper headers (function_detail.h, function.h, algorithm.h) require new parser features

## Suite Status
- Before: 2114/2114
- After: 2115/2115 (all passing, 0 regressions, +1 new test)
