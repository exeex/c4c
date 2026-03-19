# Plan Execution State

## Active Plan
- Parent: `plan.md` (STL Enablement Umbrella)
- Active child: `iterator_plan.md` — Phases 0–5 complete (range-for landed)
- Active child: `range_for_plan.md` — Phase 0–1 complete
- Previous child: `operator_overload_plan.md` — Phase 4 complete, Phase 5 deferred

## Current Target
- **Milestone 3**: DONE (container_plan.md Phase 0 — fixed_vec_smoke.cpp passes)
- **Next target**: TBD — see "Next Intended Slice" section

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

- Phase 4: Iterator/container ergonomics
  - **const_iterator pattern**: ConstIter type with const int* field, const operator* and operator!=, container with const begin/end returning ConstIter, sum through const pointer
  - **operator-> chaining**: C++ compliant chaining — when operator->() returns a struct (not pointer), recursively calls that struct's operator->() until a raw pointer is obtained. Intermediate struct results stored in temp locals for addressability.
  - **Nested type aliases (typedef in struct)**: Parser handles `typedef` inside struct body in C++ mode. Registers both plain name (usable inside struct methods) and scoped `StructName::TypeName` (usable outside). Added `ColonColon` token to lexer, `StructName::TypeName` resolution in `parse_base_type()` and `is_type_start()`.
  - **Bugfixes**:
    - `infer_generic_ctrl_type` NK_DEREF now checks both `operator_deref` and `operator_deref_const` in `struct_method_ret_types_`
    - `try_lower_operator_call` falls back to `struct_method_ret_types_` when `fn_index` doesn't have the method yet (methods not lowered before main)
  - Tests:
    - `const_iterator_basic.cpp` — ConstIter with const int*, const methods, const container access
    - `iterator_arrow_chain_basic.cpp` — Outer->Wrapper->Inner* chaining
    - `iterator_alias_basic.cpp` — typedef Iter iterator inside Container, Container::iterator usage
  - Suite: 1939/1939 (was 1936)

- Phase 5: Range-for integration
  - **Created `range_for_plan.md`**: child plan for range-for syntax support
  - **NK_RANGE_FOR node kind**: new AST node for `for (Type var : range_expr) body`
  - **Parser**: detects range-for in C++ mode by parsing decl then checking for `:` vs `;`. Uses save/restore of parser position for backtracking if not range-for.
  - **HIR desugaring**: `NK_RANGE_FOR` lowered to equivalent of:
    - `__range_begin = range_expr.begin()`
    - `__range_end = range_expr.end()`
    - `ForStmt { cond: __begin != __end, update: ++__begin }`
    - Body: `Type var = *__begin; user_body`
  - **Sema**: `NK_RANGE_FOR` handled in validate.cpp (visits decl, range expr, body with loop_depth)
  - **Consteval**: `NK_RANGE_FOR` returns unsupported error (not needed for consteval)
  - Tests:
    - `range_for_basic.cpp` — sum elements of Container using `for (int x : c)`, two loops over same container
    - `range_for_const.cpp` — range-for with const begin/end methods, ConstIter with const operator*
  - Suite: 1941/1941 (was 1939)

### Range-For Plan Phase 1 (range_for_plan.md)
- **TB_AUTO type base**: added to TypeBase enum as placeholder for auto deduction
- **Parser**: In C++ mode, `KwAuto` sets `TB_AUTO` instead of being ignored as storage class
- **HIR lowering**: In NK_RANGE_FOR, when `decl_node->type.base == TB_AUTO`, deduces loop variable type from `operator*` return type. Preserves const qualifier from declaration (`const auto`).
- Tests:
  - `range_for_auto.cpp` — `for (auto x : c)` with int elements, two loops
  - `range_for_const_auto.cpp` — `for (const auto x : c)` with const begin/end
- Suite: 1943/1943 (was 1941)

### Container Plan Phase 0 (container_plan.md)
- **container_plan.md**: created child plan for Milestone 3 (small vector-like usability)
- **fixed_vec_smoke.cpp**: comprehensive smoke test exercising all Milestone 3 APIs
  - FixedVec with buf[16]+len, Iter with int* field
  - `size()`, `empty()`, `data()`, `front()`, `back()`
  - `operator[]` non-const (returns int*) and const (returns int)
  - `push_back(int)`, `pop_back()`
  - `begin()` / `end()` returning Iter
  - `sum_manual()` via iterator loop, `sum_range()` via `for (auto x : *v)`
  - All 23 assertion points pass — no compiler changes needed
- Suite: 1944/1944 (was 1943)
- **Result**: Milestone 3 validated — all landed features (operators, iterators, range-for, auto) compose correctly for vector-like container code

### Range-For Plan Phase 1 slice 2 (range_for_plan.md)
- **Parser**: `&` between return type and `operator` keyword (e.g., `int& operator*()`)
- **Sema**: Skip lvalue-ref "must be initialized" check for range-for loop variable decl
- **HIR lowering**: NK_RETURN wraps in AddrOf when function returns T& (reference)
- **HIR lowering**: Range-for auto deduction strips is_lvalue_ref from deref_ret_ts before applying user's auto& qualifier
- **HIR lowering**: Reference loop variable uses direct call result (no AddrOf) when operator* returns T&
- **Codegen**: `llvm_ret_ty()` helper — returns "ptr" for T& types in function signatures
- **Codegen**: Function signatures, ret instructions, call instructions use `llvm_ret_ty`
- **Codegen**: `resolve_payload_type` for CallExpr bumps ptr_level when return type is T&
- Tests:
  - `range_for_auto_ref.cpp` — `for (auto& x : c)` mutating elements, `for (const auto& x : c)` reading, `&x` taking address
- Suite: 1945/1945 (was 1944)

### Rvalue Reference Plan Phase 0 (rvalue_ref_plan.md)
- **`is_rvalue_ref` in TypeSpec**: new boolean field for T&& types
- **Parser**: `&&` (AmpAmp) in declarators sets `is_rvalue_ref` in C++ mode — handles direct declarators, parenthesized declarators, and method return types
- **Type utilities**: `is_rvalue_ref_ty`, `remove_rvalue_ref`, `add_rvalue_ref`, `is_any_ref_ty`, `remove_any_ref` helpers
- **`ref_storage_type`**: updated to also bump ptr_level for rvalue refs (stored as pointers like lvalue refs)
- **Sema validation**: `is_rvalue_ref` in type comparison; rvalue refs must be initialized; `decay_array` / `referred_type` clear rvalue ref
- **Canonical type system**: `RValueReference` kind added; mangled as `O` (Itanium ABI); full round-trip through `typespec_from_canonical`
- **HIR lowering**: `is_any_ref_ts()` helper; rvalue ref locals materialize temp + store address; rvalue ref params materialize temp for rvalue args; return/declref paths handle rvalue refs
- **Codegen**: `llvm_ret_ty` returns "ptr" for rvalue refs; coerce handles ref-dereference (load) when ref return value is assigned to non-ref variable
- **Bugfix**: Reference-returning function result assigned to non-ref variable now emits `load` instead of `ptrtoint` (fixes both lvalue and rvalue ref cases)
- Tests:
  - `rvalue_ref_decl_basic.cpp` — `int&& r = 42`, modify through ref, const rvalue ref
  - `rvalue_ref_param_basic.cpp` — `int&&` function parameters, passing rvalues
- Suite: 1947/1947 (was 1945)

### Rvalue Reference Plan Phase 1 (rvalue_ref_plan.md)
- **Sema validation**: rvalue reference binding rules enforced at three sites:
  - Global declarations: `is_rvalue_ref && rhs.is_lvalue` → error
  - Local declarations: `is_rvalue_ref && rhs.is_lvalue` → error
  - Function call arguments: `is_rvalue_ref && arg.is_lvalue` → error
- **Diagnostics**: "rvalue reference cannot bind to an lvalue" (decl), "rvalue reference parameter cannot bind to an lvalue argument" (call)
- Tests:
  - `rvalue_ref_bind_literal.cpp` — int&&, long&&, char&&, const int&& binding literals and expressions
  - `rvalue_ref_bind_temp.cpp` — T&& binding function return values and cast expressions
  - `rvalue_ref_param_call_basic.cpp` — T&& params with rvalue args (literals, expressions)
  - `bad_rvalue_ref_bind_lvalue.cpp` — `int&& r = x` rejected
  - `bad_rvalue_ref_param_lvalue.cpp` — `consume(x)` with `int&&` param rejected
- Suite: 1952/1952 (was 1947)

### Rvalue Reference Plan Phase 2 (rvalue_ref_plan.md)
- **Sema**: `canonicalize_param_type` preserves ref qualifiers across `decay_array` (was stripping them)
- **Sema**: `is_ref_overload()` helper detects function declarations that differ only in ref-qualifier params
- **Sema**: `ref_overload_sigs_` stores multiple overloads per function name; call validation picks best viable overload by argument value category scoring
- **HIR**: Phase 1.9 pre-scan detects ref-overloaded free functions; second overload gets `__rref_overload` suffix
- **HIR**: Struct method ref-overloads detected during method collection; second overload gets `__rref` suffix
- **HIR**: `resolve_ref_overload()` at call sites determines arg lvalue/rvalue status via `is_ast_lvalue()`, picks best overload by score
- **HIR**: Method call dispatch uses resolved overload name for callee lookup and param coercion
- Tests:
  - `ref_overload_lvalue_vs_rvalue.cpp` — free function `f(int&)` vs `f(int&&)`, lvalue/rvalue dispatch
  - `ref_overload_method_basic.cpp` — struct method `set(int&)` vs `set(int&&)`, lvalue/rvalue dispatch
  - `ref_overload_method_reads_arg.cpp` — method ref-overload call must pass the bound object's address, not coerce the argument value to a pointer
  - `ref_overload_const_lvalue_vs_rvalue.cpp` — `f(const int&)` vs `f(int&&)`, rvalue prefers T&&
- Suite: 1955/1955 (was 1952)

### Named Cast Coverage
- `static_cast_basic.cpp` — baseline `static_cast<T>(expr)` coverage for scalar conversions, expression contexts, consteval calls, and template code
- `static_cast_rvalue_ref_basic.cpp` — `static_cast<T&&>(lvalue)` coverage for rvalue-ref overload selection and `int&&` parameter binding
  - Current observation: this test passes, and `static_cast<int&&>(a)` is treated as an rvalue for overload resolution/binding
  - IR limitation: lowering currently materializes a fresh temporary from `a` and passes the temp address, instead of treating `a` itself as an xvalue view
  - Clang comparison: Clang passes the original object's address for `static_cast<int&&>(a)`; our IR currently does `load a -> store temp -> pass &temp`
  - Recommended fix:
    - Teach sema/HIR to distinguish "same-object xvalue" from ordinary prvalue temporaries
    - Special-case `static_cast<T&&>(lvalue)` so it preserves object identity instead of forcing materialization
    - Update ref-overload resolution to use value category metadata rather than simple AST-kind heuristics when a cast produces xvalue
    - Lower xvalue reference arguments by passing the original object's address directly; keep temp materialization only for true prvalues
    - Add a follow-up regression where mutating through `int&&` after `static_cast<int&&>(a)` is observed through `a`, to prove same-object binding

### Rvalue Reference Plan Phase 3 (rvalue_ref_plan.md)
- **Constructor declaration parsing**: `ClassName(params)` recognized in struct body when identifier matches `current_struct_tag_` followed by `(`; sets `is_constructor=true` on NK_FUNCTION node; return type implicitly void
- **Constructor invocation syntax**: `Type var(args)` in C++ mode parses constructor arguments (not K&R fn decl) when base type is struct/typedef
- **Constructor overload resolution**: `struct_constructors_` map stores all constructors per struct tag with unique mangled names (`Tag__Tag`, `Tag__Tag__1`, etc.); scoring considers param count, base type match, and ref-qualifier/value-category matching (T&& prefers rvalue, const T& prefers lvalue)
- **operator= support**: `OP_ASSIGN` added to OperatorKind; parser recognizes `operator=`; NK_ASSIGN on struct types dispatches to `try_lower_operator_call` before falling back to memcpy
- **Reference param handling in operator calls**: `try_lower_operator_call` now detects rvalue/lvalue ref params (via fn_index or fallback to pending AST node) and passes address instead of value; `static_cast<T&&>(x)` unwrapped to pass `&x` directly (xvalue semantics)
- **Node additions**: `is_constructor` (NK_FUNCTION), `is_ctor_init` (NK_DECL) flags in ast.hpp
- Tests:
  - `constructor_basic.cpp` — `Box(int v)` constructor declaration and `Box b(42)` invocation
  - `move_ctor_basic.cpp` — `Obj(Obj&&)` move constructor, `static_cast<Obj&&>` to trigger, source marked moved-from
  - `move_assign_basic.cpp` — `Obj& operator=(Obj&&)` move assignment operator with `static_cast<Obj&&>`
  - `move_ctor_over_copy.cpp` — copy ctor `Obj(const Obj&)` vs move ctor `Obj(Obj&&)`, lvalue selects copy, rvalue selects move
- Suite: 1961/1961 (was 1957)

### Rvalue Reference Plan Phase 4 (rvalue_ref_plan.md)
- **Template move helper**: `template <typename T> T&& my_move(T& x)` works end-to-end
  - **emit_lval CastExpr**: reference-type casts (T&&, T&) now preserve addressability (xvalue semantics) — `static_cast<T&&>(x)` returns same address as x
  - **infer_generic_ctrl_type NK_CALL**: added case for function call return type inference, supports deduced template calls and direct fn_index lookup
  - **Constructor arg ref-returning call**: when arg to reference param is a function call returning T&/T&&, pass call result directly (already a pointer) instead of wrapping in AddrOf
  - **Sema is_ctor_init**: constructor-initialized variables now marked as initialized in sema validator (fixes false "uninitialized variable" on `T result(ctor_args)`)
- Tests:
  - `move_helper_basic.cpp` — template `my_move()` triggers move ctor, source marked moved-from
  - `template_rvalue_ref_param_basic.cpp` — scalar T&& binding, chained move ctor calls
- Suite: 1963/1963 (was 1961)

### Rvalue Reference Negative Tests (rvalue_ref_plan.md Phases 3-4)
- **`= delete` support**: `KwDelete` token, `is_deleted` Node flag, parser handles `= delete` after constructor/method declarations, HIR rejects calls to deleted constructors
- **Deleted methods skip lowering**: `lower_struct_method` returns early for `is_deleted` methods
- Tests:
  - `bad_deleted_move_ctor_call.cpp` — `Obj(Obj&&) = delete` then `Obj b(static_cast<Obj&&>(a))` rejected
  - `bad_move_assign_const.cpp` — move-assignment to const object rejected (already worked)
  - `bad_forwarding_ref_bind.cpp` — non-template `int&&` param rejects lvalue arg (already worked)
- Suite: 1966/1966 (was 1963)

### Default Constructor (`T()`)
- **Implicit default ctor call**: When `T var;` is declared (no init, no is_ctor_init) and T is a struct with a zero-arg constructor in `struct_constructors_`, HIR now emits the constructor call automatically
- **Deleted default ctor check**: If the zero-arg ctor has `is_deleted=true`, throws error
- Tests:
  - `default_ctor_basic.cpp` — Counter/Pair with default ctor, T var; calls it, overload with parameterized ctor
  - `bad_deleted_default_ctor.cpp` — `T() = delete` then `T var;` rejected
- Suite: 1968/1968 (was 1966)

### Destructor Support (`~ClassName()`)
- **Parser**: `~ClassName()` recognized in struct body when `Tilde` + `Identifier` matching `current_struct_tag_` + `()`; sets `is_destructor=true` on NK_FUNCTION node
- **`= delete` support**: destructors can be `= delete`; attempting to declare a variable of such type is rejected
- **HIR registration**: `struct_destructors_` map stores one destructor per struct tag with mangled name `Tag__dtor`
- **Implicit destructor calls**: scope-exit and return-path destructor emission via `dtor_stack` in FunctionCtx
  - NK_BLOCK: saves dtor_stack depth on entry, emits dtor calls (reverse order) on exit
  - NK_RETURN: emits dtor calls for all locals in scope before the return statement
- **DtorLocal tracking**: struct locals with destructors are pushed to `ctx.dtor_stack` during `lower_local_decl_stmt`
- Tests:
  - `destructor_basic.cpp` — Widget with `~Widget()`, single scope, multiple objects (reverse order), nested scopes
  - `bad_deleted_destructor.cpp` — `~NoDtor() = delete` then `NoDtor n;` rejected
- Suite: 1970/1970 (was 1968)

### Copy Assignment Operator (`operator=(const T&)`)
- **Ref-overload resolution in `try_lower_operator_call`**: When an operator method has both `const T&` and `T&&` overloads (stored in `ref_overload_set_`), the function now scores argument value categories (lvalue vs rvalue) to pick the correct overload, matching the logic in `resolve_ref_overload()` for regular method calls.
- **Pending method lookup fix**: AST fallback for parameter types now uses `resolved_mangled` instead of `mit->second` (which was overwritten by the second overload during registration).
- Tests:
  - `copy_assign_basic.cpp` — `Obj& operator=(const Obj&)`, lvalue copy, source unchanged
  - `copy_move_assign_overload.cpp` — both `operator=(const T&)` and `operator=(T&&)` coexist; lvalue selects copy, rvalue selects move
  - `copy_assign_const_src.cpp` — copy from `const Data` source, multi-field struct
- Suite: 1973/1973 (was 1970)

### Constructor Initializer Lists (`: member(init), ...`)
- **Parser**: After `expect(RParen)` in constructor parsing, check for `Colon` token; parse `member(expr), ...` sequence into `ctor_init_names[]` + `ctor_init_exprs[]` arrays
- **AST**: Added `ctor_init_names` (const char**), `ctor_init_exprs` (Node**), `n_ctor_inits` (int) to Node struct
- **HIR lowering**: In `lower_struct_method`, before `lower_stmt_node(body)`, emit member assignments: `this->member = expr` for each init list item, using struct field types from `module_->struct_defs`
- **Sema**: Validate initializer list expressions via `infer_expr()` after params are bound
- Tests:
  - `ctor_init_list_basic.cpp` — Point(a,b):x(a),y(b), Rect 4-member, Counter with expressions, Mixed init-list + body
- Suite: 1974/1974 (was 1973)

### Member Constructor Calls in Initializer Lists
- **Parser**: Init list now parses comma-separated args per member: `member(a, b, ...)` and zero-arg `member()`
- **AST**: Changed `ctor_init_exprs` → `ctor_init_args` (Node***) + `ctor_init_nargs` (int*) for multi-arg support
- **HIR lowering**: Detects struct-typed members with constructors in `struct_constructors_`; emits `MemberTag__MemberTag(&this->member, args...)` instead of simple assignment; includes overload resolution with ref-qualifier scoring
- **Sema**: Validates all per-init args via `infer_expr()`
- Tests:
  - `ctor_init_member_ctor.cpp` — Inner(int), Pair(int,int) members; Wrapper/Container/Box/TwoInner/Hybrid patterns
- Suite: 1975/1975 (was 1974)

### Member Destructor Calls for Struct-Typed Members
- **`emit_member_dtor_calls` helper**: emits destructor calls for all struct-typed member fields in reverse field order; GEPs into `this->field` and calls `Tag__dtor(ptr)` for each
- **`struct_has_member_dtors` helper**: recursively checks if any member field's type has a destructor (explicit or via its own members)
- **User-defined dtor body epilogue**: after `lower_stmt_node(body)` for `is_destructor` methods, emits member dtor calls via `emit_member_dtor_calls`
- **Implicit member dtor tracking**: structs without explicit dtor but with member dtors are pushed to `dtor_stack`; `emit_dtor_calls` calls `emit_member_dtor_calls` directly for them
- **No double-call**: when a field has an explicit dtor, only the dtor is called (it handles its own members); recursive member emission only for fields without explicit dtor
- Tests:
  - `destructor_member_basic.cpp` — Outer with two Inner members (user dtor + member dtors), Wrapper with no dtor (implicit member dtor), Top→Middle→Inner nested chain
- Suite: 1976/1976 (was 1975)

### operator() (Function Call Operator)
- **OP_CALL enum value**: added to OperatorKind in ast.hpp
- **Parser**: `operator()` recognized — consumes `(` `)` after `operator` keyword, then parses normal parameter list
- **Mangled name**: `operator_call` via `operator_kind_mangled_name()`
- **HIR dispatch**: In `lower_call_expr`, when callee is NK_VAR resolving to a struct type, tries `try_lower_operator_call(ctx, n, n->left, "operator_call", arg_nodes)` before regular call handling
- **Type inference**: `infer_generic_ctrl_type` for NK_CALL checks `struct_method_ret_types_` for `Tag::operator_call` when callee is struct
- Tests:
  - `operator_call_basic.cpp` — Adder(int x), Multiplier(int a, int b), Counter() zero-arg; all return int
- Suite: 1977/1977 (was 1976)

### Comparison Operators (`operator<`, `operator>`, `operator<=`, `operator>=`)
- **OP_LT, OP_GT, OP_LE, OP_GE enum values**: added to OperatorKind in ast.hpp
- **Parser**: `<`, `>`, `<=`, `>=` recognized after `operator` keyword (LessEqual/GreaterEqual checked before Less/Greater)
- **Mangled names**: `operator_lt`, `operator_gt`, `operator_le`, `operator_ge`
- **HIR dispatch**: NK_BINOP maps `<`, `>`, `<=`, `>=` ops to `try_lower_operator_call` on struct types
- Tests:
  - `operator_compare_basic.cpp` — Val with all 4 comparison operators + == and !=, 16 assertion points
- Suite: 1978/1978 (was 1977)

## Next Intended Slice
### Recommended next target
- Next priority:
  1. `operator_overload_plan.md` Phase 5: free-function operators (if real tests need them)
  2. Template member access through T&& params (blocked on template member resolution)
  3. Delegating constructors

### Explicitly deferred for now
- Free-function operator overloading
- Structured bindings in range-for
- General `auto` deduction outside range-for
- Ambiguous overload detection (bad_ref_overload_ambiguous.cpp)
- Template function T&& param member access (e.g. `obj.value` where obj is T&&)

## Known Limitations
- No free-function (non-member) operators
- No parameter-type-based overload resolution for same operator (e.g., `operator-(int)` vs `operator-(Iter)`)
- Const method dispatch relies on TypeSpec.is_const — const reference parameters work but requires proper const propagation through the type system
- Nested typedef names are also registered globally (potential conflicts if two structs define same typedef name)
- No `using` alias declarations (only `typedef`)
- `auto` type deduction only works in range-for context, not general variable declarations
- Milestone 3 child plan: `container_plan.md` (Phase 0 complete)
- No full `static_cast` implementation model yet; only baseline named-cast coverage is tracked
- Constructor overload resolution is basic — scores by param count + base type + ref-qualifier, no implicit conversions across types
- Constructor initializer list does not support delegating constructors (only scalar/member-ctor init)

## Notes
- try_lower_operator_call() builds CallExpr with &obj as implicit this + explicit args
- Uses ExprId::invalid() sentinel (not ExprId{} which has value 0 = valid first expr)
- n->op for prefix unary is "++pre"/"--pre", not "++"/"--"
- resolve_typedef_to_struct() needed because parser can't add full typedef resolution before struct body is complete (causes "incomplete type" errors for local vars)
- maybe_bool_convert() inserted at: if/while/do-while/for cond, ternary cond, logical !, &&, ||
- Conversion operators (operator bool) have no return type prefix — parser detects KwOperator directly
- struct_method_ret_types_ map stores return types at registration time (before fn_index is available)
- Const methods use "_const" suffix: mangled="Struct__method_const", key="Struct::method_const"
- ColonColon token only emitted in CppSubset lex profile; C mode unaffected
- current_struct_tag_ tracks active struct during parsing for scoped typedef registration
- NK_RANGE_FOR desugaring: parser saves pos_ + typedefs_ for backtracking; HIR builds method calls directly using struct_methods_ map
- Constructor detection: identifier == current_struct_tag_ + LParen in struct body → is_constructor=true method
- Constructor invocation: Type var(args) in C++ mode → is_ctor_init=true on NK_DECL, args in children[]
- Constructor overloads: struct_constructors_ map stores all ctors per tag; scoring considers ref-qualifier match
- operator= dispatched before fallback memcpy in NK_ASSIGN; uses try_lower_operator_call("operator_assign")
- try_lower_operator_call now handles ref params: falls back to pending method AST node for param types when fn_index not yet populated
- Constructor initializer list: parsed after RParen before LBrace; per-init multi-arg support; struct members dispatched to constructor calls, scalar members use direct assignment
