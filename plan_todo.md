# Plan Execution State

## Active Item
Operator Overloading — item 4a: finish `operator new/delete/new[]/delete[]`
family for non-stdlib C++ mode, then return to EASTL/std_vector bring-up

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

## Next Slice
- EASTL bring-up slice 7b: deeper header errors now led by:
  - `EASTL/allocator.h` placement/global-qualified `new` expressions
  - `EASTL/internal/function_detail.h` placement-new / destructor / qualified fn patterns
  - `delete` / `delete[]` expressions still use placeholder lowering
  - class-specific vs global `operator new/delete` lookup is not wired for expressions yet
  - `EABase/int128.h` constructor-style expressions with multiple args
  - `EASTL/internal/tuple_fwd_decls.h` templated `get(...)` overloads
  - `EASTL/utility.h` deeper piecewise ctor / tuple trait uses

## Exposed Failing Tests
- Parse targets intentionally registered in `ctest -j` and currently failing:
  - `cpp_positive_sema_new_expr_basic_parse_cpp`
  - `cpp_positive_sema_placement_new_expr_parse_cpp`
  - `cpp_positive_sema_class_specific_new_delete_parse_cpp`
  - Current failure shape:
    - plain/class-specific `new`: `unexpected token in expression: new`
    - placement `::new`: `unexpected token in expression: ::`
- Runtime targets intentionally registered in `ctest -j` and currently failing:
  - `cpp_positive_sema_new_delete_side_effect_runtime_cpp`
  - `cpp_positive_sema_class_specific_new_delete_side_effect_runtime_cpp`
  - `cpp_positive_sema_new_array_delete_array_side_effect_runtime_cpp`
  - Each test encodes observable side effects in user-defined `operator new/delete`
    bodies via counters; once expression support lands, these become the first
    correctness signal that lookup + execution really happened.
  - Current failure shape:
    - frontend rejects `new` expressions before runtime side effects can be checked

## Blockers
- `new` / placement-`new` expression support is now the first real EASTL blocker
- `delete` / `delete[]` expression semantics are still incomplete

## Suite Status
- Before: 2105/2105 (+ 3 pre-existing env failures)
- After:
  - `operator_new_delete_overload_parse.cpp` is green and covers declarators +
    explicit operator-function calls
  - 3 parse targets and 3 side-effect runtime targets are intentionally red in
    `ctest -j` to expose the remaining `new/delete` expression gap
