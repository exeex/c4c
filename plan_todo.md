# Plan Execution State

## Active Plan
- Parent: `plan.md` (STL Enablement Umbrella)
- Active child: `operator_overload_plan.md` — Phase 1: Member-call semantic resolution

## Current Target
- **Phase 1, Slice 1**: DONE

## Completed Items
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

## Next Intended Slice
- Phase 1, Slice 2: Struct-by-value argument passing for operator methods
  - `operator==(Val other)` where Val is a struct — currently limited to int params
  - Requires codegen-level struct-to-scalar ABI coercion in operator call args
- Phase 2: Lowering completeness — postfix operator++, operator->, operator bool (implicit conversion)
- Phase 3: Iterator-critical operators

## Known Limitations
- Self-referencing struct types as RETURN types work (via injected class name)
- Self-referencing struct types as local VARIABLE types inside inline method bodies fail ("incomplete type") because method bodies are parsed inline (C++ defers body parsing to after class completion)
- Struct-by-value operator parameters require ABI coercion not yet handled in operator call path
- operator bool implicit conversion in boolean contexts (`if (obj)`) not yet supported

## Notes
- try_lower_operator_call() builds CallExpr with &obj as implicit this + explicit args
- Uses ExprId::invalid() sentinel (not ExprId{} which has value 0 = valid first expr)
- n->op for prefix unary is "++pre"/"--pre", not "++"/"--"
