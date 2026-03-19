# Plan Execution State

## Active Plan
- Parent: `plan.md` (STL Enablement Umbrella)
- Active child: `iterator_plan.md` — Phases 0–3 complete
- Previous child: `operator_overload_plan.md` — Phase 4 complete, Phase 5 deferred

## Current Target
- **Iterator Plan Phases 0–3**: DONE

## Completed Items

### Operator Overload Plan (operator_overload_plan.md)
- Phase 0, Slice 1: KwOperator token + operator-function declarator parsing + parse tests
  - Added `KwOperator` token kind to lexer
  - Added `OperatorKind` enum to ast.hpp (OP_SUBSCRIPT, OP_DEREF, OP_ARROW, OP_PRE_INC, OP_POST_INC, OP_EQ, OP_NEQ, OP_BOOL, OP_PLUS, OP_MINUS)
  - Added `operator_kind` field to Node struct
  - Parser recognizes `operator[]`, `operator*`, `operator->`, `operator++`, `operator==`, `operator!=`, `operator bool`, `operator+`, `operator-` in struct method declarations
  - Prefix vs postfix `operator++` distinguished by parameter count (0 = prefix, 1+ = postfix)
  - `operator bool` sets return type to TB_BOOL automatically
  - Canonical mangled names: `operator_subscript`, `operator_deref`, etc.
  - 7 positive parse tests + 1 negative test (bad_operator_unknown_token)
  - Suite: 1908/1908 (was 1900)

- Phase 1, Slice 1: Operator expression → member method call rewriting + runtime tests
  - **Parser fix**: Injected class name — register struct tag in `typedefs_` before body parsing, enabling self-referencing return types (e.g., `Counter operator++()`)
  - **HIR lowering**: `try_lower_operator_call()` helper detects operator expressions on struct types and rewrites to member method calls
  - Supported expression-to-call rewrites:
    - `NK_INDEX` (a[i]) on struct → `operator_subscript` method call
    - `NK_DEREF` (*a) on struct → `operator_deref` method call
    - `NK_UNARY` (++a / --a) on struct → `operator_preinc` / `operator_predec` method call
    - `NK_BINOP` (a==b / a!=b / a+b / a-b) on struct → `operator_eq` / `operator_neq` / `operator_plus` / `operator_minus` method call
  - 4 positive runtime tests:
    - `operator_subscript_member_basic.cpp` — Vec with `operator[](int)`
    - `operator_deref_member_basic.cpp` — IntPtr with `operator*()`
    - `operator_preinc_member_basic.cpp` — Counter with void `operator++()`
    - `operator_eq_member_basic.cpp` — Val with `operator==(int)` and `operator!=(int)`
  - Suite: 1912/1912 (was 1908)

- Phase 1, Slice 2: Struct-by-value parameter/return types for struct methods
  - **Root cause**: Injected class name registered in `typedefs_` (recognition) but not `typedef_types_` (resolution) before struct body parsing. Method parameters/returns typed as `TB_TYPEDEF` with self-struct tag stayed unresolved in HIR.
  - **Fix**: `resolve_typedef_to_struct()` helper in ast_to_hir.cpp resolves `TB_TYPEDEF` to `TB_STRUCT` when the tag matches a known struct_def. Applied to:
    - Method return types
    - Method parameter types
    - Local variable declarations inside method bodies
  - **Test**: `operator_struct_byval_param.cpp` — Vec2 with `operator==(Vec2)`, `operator!=(Vec2)`, `operator+(Vec2)`, `operator-(Vec2)` all passing struct by value and returning struct
  - Suite: 1913/1913 (was 1912)

- Phase 2/3 combined: Postfix operator++, operator->, operator bool
  - **Postfix operator++(int)**: Added dispatch in NK_POSTFIX to `try_lower_operator_call` with dummy int 0 arg. Extended `try_lower_operator_call` to accept optional pre-lowered `extra_args`.
  - **operator->**: Added dispatch in NK_MEMBER when `is_arrow` on struct types. Calls `operator_arrow()` which returns a pointer, then applies arrow field access on the result. Fixed parser to consume `*` pointer declarator tokens between base type and `operator` keyword (e.g., `Inner* operator->()`).
  - **operator bool**: Fixed parser to handle conversion operators where `operator` appears directly without a return type prefix (added `is_conversion_operator` early detection before `is_type_start()` guard). Added `maybe_bool_convert()` helper that detects struct types with `operator_bool` and inserts implicit call. Applied at all boolean context sites: if, while, do-while, for conditions; ternary condition; logical NOT (`!`); logical AND/OR (`&&`/`||`).
  - Tests:
    - `operator_postinc_member_basic.cpp` — Counter with `operator++(int)` returning old value
    - `operator_arrow_member_basic.cpp` — Wrapper with `operator->()` returning Inner*
    - `operator_bool_member_basic.cpp` — Handle with `operator bool()` in if/while/! contexts
  - Suite: 1916/1916 (was 1913)

- Phase 4: Container-facing operators
  - **Const method qualifier support**: Added `is_const_method` flag to Node (ast.hpp). Parser captures trailing `const` on methods (both operator and regular). Const methods get `_const` suffix in mangled name and lookup key.
  - **Const/non-const method dispatch**: Updated `try_lower_operator_call()`, `maybe_bool_convert()`, and direct method call dispatch to select between const and non-const overloads. Const objects prefer const overload; non-const objects prefer non-const, falling back to const.
  - **operator_deref type inference fix**: `infer_generic_ctrl_type` for NK_DEREF on struct with `operator_deref` now returns the operator's return type instead of the struct type. Uses `struct_method_ret_types_` map (populated at method registration) since fn_index isn't available during early type inference.
  - Tests:
    - `container_subscript_pair.cpp` — Vec with `int* operator[](int)` and `int operator[](int) const`, plus `size() const`
    - `container_bool_state.cpp` — Handle and OptionalInt with `operator bool()` in various boolean contexts
    - `iterator_plus_basic.cpp` — Iter with `operator+(int)` and `operator-(int)` for stepping, combined with `operator*()` and `operator!=(Iter)`
    - `iterator_minus_basic.cpp` — Iter with `operator-(int)` for backward stepping
    - `manual_iterator_loop.cpp` — Full integration test: Container with begin()/end(), Iter with operator*/++/!=, manual while loop summing elements
  - Suite: 1921/1921 (was 1916)

### Iterator Plan (iterator_plan.md)
- Phase 0: Iterator model and test contract
  - `iterator_contract_basic.cpp` — validates operator*, prefix ++, !=, == on standalone Iter
  - `container_begin_end_manual_loop.cpp` — for loop with begin/end + iterator operators
  - All operators and iterator shape work correctly
  - Suite: 1936/1936 (was 1921)

- Phase 1: Iterator object basics
  - `iterator_class_basic.cpp` — Iter with fields, method calls (value(), at_end())
  - `iterator_return_from_method.cpp` — returning Iter from container methods
  - `iterator_copy_basic.cpp` — copy init, copy assignment, independence after copy
  - `iterator_const_method_basic.cpp` — const methods on iterator type
  - All iterator object semantics work correctly

- Phase 2: Operator-dependent iterator surface
  - `iterator_deref_basic.cpp` — operator* returning int and int*
  - `iterator_preinc_basic.cpp` — prefix ++ stepping through elements
  - `iterator_neq_basic.cpp` — operator!= for loop termination
  - `iterator_eq_basic.cpp` — operator== for equality checks
  - `iterator_arrow_basic.cpp` — operator-> for field access on Point structs
  - All required iterator operators work correctly

- Phase 3: Container integration
  - `begin_end_member_basic.cpp` — basic begin/end returning iterators
  - `const_begin_end_member.cpp` — const begin/end members
  - `iterator_pair_loop.cpp` — classic for(it=begin; it!=end; ++it) pattern
  - `container_iterator_smoke.cpp` — full smoke: multiple containers, sum_all helper, empty container
  - All container integration patterns work correctly

## Next Intended Slice
- Iterator Plan Phase 4: Iterator/container ergonomics (operator->, const_iterator, nested aliases)
- Or: Operator Overload Phase 5 (free-function operators) if needed
- Or: Range-for plan (Phase 5 of iterator plan)

## Known Limitations
- operator-> chaining (e.g., `a->b->c` where both a and b have operator->) not yet supported
- No free-function (non-member) operators
- No operator() (function call operator)
- No parameter-type-based overload resolution for same operator (e.g., `operator-(int)` vs `operator-(Iter)`)
- Const method dispatch relies on TypeSpec.is_const — const reference parameters work but requires proper const propagation through the type system
- No `auto` type deduction — iterator tests use explicit type names

## Notes
- try_lower_operator_call() builds CallExpr with &obj as implicit this + explicit args
- Uses ExprId::invalid() sentinel (not ExprId{} which has value 0 = valid first expr)
- n->op for prefix unary is "++pre"/"--pre", not "++"/"--"
- resolve_typedef_to_struct() needed because parser can't add full typedef resolution before struct body is complete (causes "incomplete type" errors for local vars)
- maybe_bool_convert() inserted at: if/while/do-while/for cond, ternary cond, logical !, &&, ||
- Conversion operators (operator bool) have no return type prefix — parser detects KwOperator directly
- struct_method_ret_types_ map stores return types at registration time (before fn_index is available)
- Const methods use "_const" suffix: mangled="Struct__method_const", key="Struct::method_const"
