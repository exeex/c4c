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
- **Milestone B Phase 4: Negative test runner with expected-error support** — Added `verify_case/` test directory with 5 curated tests using `expected-error {{...}}` annotations. Created `run_verify_case.cmake` that invokes `verify_external_clang_case.py` in verify mode. Registered verify tests in `InternalTests.cmake` with `verify_case` label. Tests cover: statement recovery, empty initializer, top-level recovery, break/continue outside loop, call arity mismatch. All 2028/2028 tests pass.
- **Milestone B Phase 5: Curate external C clang allowlists** — Triaged all 144 external C clang test files into buckets:
  - 37 clean pass (no expected-error, rc=0) → added as `pass`
  - 1 verify pass (C/C23/n2359.c) → added as `verify`
  - 76 deferred (expected-error annotations too Clang-specific to match)
  - 30 positive tests our compiler fails on (our bugs, not added)
  Allowlist grew from 1 to 38 entries. Test suite: 2065/2065 (100%).

- **Namespace Phase 4 slice 2: Function name lookup across namespaces** — Added `known_fn_names_` set to parser; functions register their qualified name when declared/defined. `lookup_value_in_context()` now checks both `var_types_` and `known_fn_names_`, enabling unqualified calls to functions in anonymous namespaces and via `using namespace` directives. Two new runtime tests: `anon_namespace_fn_lookup.cpp`, `using_namespace_fn_lookup.cpp`. All 2067/2067 tests pass.

- **Milestone D Stage 0: LIR mechanical prep** — Created `src/codegen/lir/` directory with skeleton data structures: `LirModule`, `LirFunction`, `LirBlock`, `LirInst` (variant with 15 instruction types + `LirRawLine`), `LirTerminator` (variant with 6 terminator types). Skeleton `hir_to_lir.hpp/cpp` and `lir_printer.hpp/cpp`. CMakeLists.txt updated. All 2067/2067 tests pass.

- **Milestone D Stage 1 slice 1: Replace string sinks with LIR blocks** — Replaced `FnCtx::alloca_lines` and `body_lines` (flat `vector<string>`) with structured `lir::LirBlock` containers. `emit_lbl()` now creates new LIR blocks; `emit_instr()`/`emit_term()` push `LirRawLine` into the current block. Replaced `fn_bodies_` (`vector<FnBody>`) with `lir::LirModule module_` member in `HirEmitter`. Each function is stored as a `LirFunction` with `signature_text`, `alloca_insts`, and `blocks`. `LirFunction` gained `is_declaration`, `signature_text`, `alloca_insts` fields. Render lambda in `emit()` walks the structured blocks to produce identical LLVM IR output. Dead-code elimination updated to operate on `module_.functions`. All 2067/2067 tests pass.

- **Milestone D Stage 1 slice 2: Replace preamble_ with structured LIR containers** — Eliminated `preamble_` ostringstream from `HirEmitter`. Struct/union type definitions now stored in `module_.type_decls` (vector of pre-formatted strings). String constants (narrow and wide) stored in `module_.string_pool` (vector of `LirStringConst`). Global variable definitions stored in `module_.globals` (vector of `LirGlobal` with `raw_line` text). Intrinsic requirement flags propagated to `module_.need_*` booleans. External function declarations populated into `module_.extern_decls`. DCE global-initializer scanning now iterates `module_.globals` raw_lines. Render in `emit()` walks structured containers in order: type_decls → string_pool → globals → intrinsics → extern_decls → functions. All 2067/2067 tests pass.

- **Milestone D Stage 2: Split printer into standalone LirPrinter** — Extracted all LLVM IR rendering logic from `HirEmitter::emit()` into `lir::print_llvm()` in `lir_printer.cpp`. Added `LirSpecEntry` and `spec_entries` to `LirModule` for specialization metadata. `HirEmitter::emit()` now only performs lowering (emit_preamble, emit_global, emit_function) and module_ finalization (intrinsic flags, extern_decls, spec_entries), then delegates to `lir::print_llvm(module_)`. DCE, function/block rendering, intrinsic declarations, extern decls, string pool, and metadata are all handled by the standalone printer. All 2067/2067 tests pass.

## Active Item
**Milestone D Stage 3**: Normalize special cases into LIR ops

### Completed slices
- **Stage 3 slice 1: Typed intrinsic LIR ops** — Added `LirMemcpyOp`, `LirVaStartOp`, `LirVaEndOp`, `LirVaCopyOp`, `LirStackSaveOp`, `LirStackRestoreOp`, `LirAbsOp` to `LirInst` variant. Replaced 7 raw LLVM IR string emissions in `hir_emitter.cpp` with typed ops. `lir_printer.cpp` `render_inst` renders each op to identical LLVM IR text. All 2067/2067 tests pass.

### Next slices
- Stage 3 slice 2: Normalize bitfield extract/insert into typed LIR ops
- Stage 3 slice 3: Normalize computed goto (blockaddress + indirectbr) into typed LIR ops

## After Stage 3
- Milestone C: Iterator/container usability

## Test Suite
- Baseline: 2067/2067 (100%)

## Blockers
None known
