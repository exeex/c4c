# Plan Execution State

## Active Plan
- Parent: `plan.md` (STL Enablement Umbrella)
- Active child: `operator_overload_plan.md` — Phase 0: Syntax and naming foundation

## Current Target
- **Phase 0, Slice 1**: DONE

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

## Next Intended Slice
- Phase 1: Member-call semantic resolution — HIR lowering for operator method calls
  - Teach ast_to_hir.cpp to register operator methods in struct_methods_ map
  - Teach expression lowering to detect `obj[i]`, `*it`, `++it`, `a==b`, `a!=b` on struct types and rewrite to operator method calls
  - Add runtime tests that compile and execute operator calls

## Blockers
(none)

## Notes
- Self-referencing struct types in operator params don't parse yet (existing limitation, not specific to operators)
- operator_kind_mangled_name() returns canonical names used as method names in struct_methods_ map
