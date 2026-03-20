# Plan Execution State

## Completed
- Namespace Phase 0: Stop Digging
- Namespace Phase 1: Namespace Context Objects (parser has NamespaceContext, namespace_stack_, context management)
- Namespace Phase 2 (partial): Qualified names parsed structurally (QualifiedNameRef, qualifier_segments on Node)
- Namespace Phase 3 (partial): Declaration ownership by context (namespace_context_id on Node)
- Namespace Phase 4 (partial): Context-based lookup (lookup_value_in_context, lookup_type_in_context, using directives)
- **Namespace Phase 5 slice 1: LLVM name quoting** — `quote_llvm_ident()` and `llvm_struct_type_str()` use LLVM double-quote syntax for names with `::`. `sanitize_llvm_ident()` preserved for local vars. Dead-fn scanner handles quoted `@"name"` refs. 4 new runtime tests added.
- **Namespace Phase 5 slice 2: HIR namespace propagation** — `NamespaceQualifier` struct in ir.hpp with `segments`, `is_global_qualified`, `context_id`. Added to `DeclRef`, `Function`, `GlobalVar`, `HirStructDef`. `make_ns_qual()` helper propagates qualifier_segments and namespace_context_id from AST nodes during lowering. HIR printer displays `ns=...` annotations. All 2016/2016 tests pass.
- **Namespace Phase 2 slice 3: Namespace-qualified struct tags** — `defined_struct_tags_` and forward references now use namespace-qualified tags via `canonical_name_in_context()`. Same-named structs in different namespaces no longer collide. New runtime test `namespace_struct_collision_runtime.cpp`. All 2017/2017 tests pass.
- **Namespace Phase 5 slice 3: Nested namespace lowering + extern decl quoting** — Fixed two bugs:
  1. HIR top-level flattening was only one level deep; nested namespace blocks (NK_BLOCK inside NK_BLOCK) lost inner declarations. Fixed with recursive flattening.
  2. `extern_call_decls_` output used raw `@name` instead of `llvm_global_sym()`, producing unquoted `::` in LLVM IR declares.
  New runtime test `namespace_cross_type_reference_runtime.cpp` covers cross-namespace type references and nested namespace functions. All 2018/2018 tests pass.
- **Milestone B Phase 1+2 slice 1: Diagnostic format + invalid node kinds** — Standardized parser diagnostics to `file:line:col: error: message` format. Added `NK_INVALID_EXPR` and `NK_INVALID_STMT` to NodeKind enum as error recovery placeholders. Parser now receives source filename. HIR lowering gracefully skips invalid nodes. New diagnostic format verification test. All 2020/2020 tests pass.
- **Milestone B Phase 3: Statement-level synchronization hooks** — Added try-catch in `parse_block()` around `parse_stmt()` calls. On exception: emits `file:line:col: error:` diagnostic, skips to `;`/`}`, produces `NK_INVALID_STMT`, continues parsing. Paren-list errors inside statements are also caught by this layer. Two new negative test cases (`bad_stmt_recovery.c`, `bad_stmt_recovery_multi.c`) + cmake check. All 2023/2023 tests pass.

## Active Item
None — ready for next slice.

## Next
- Milestone B Phase 4: Negative test runner with expected-error support
- Or: Namespace Phase 2 completion (TypeSpec qualifier propagation)
- Or: Milestone C (iterator/container usability)

## Test Suite
- Baseline: 2023/2023 (100%)

## Blockers
None known
