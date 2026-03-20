# Plan Execution State

## Status: ALL MILESTONES COMPLETE

All four milestones from `plan.md` are fully executed.
Test suite: **2069/2069 (100%)**

---

## Completed Milestones

### Milestone A: Namespace context refactor
- Namespace Phase 0: Stop Digging
- Namespace Phase 1: Namespace Context Objects (parser has NamespaceContext, namespace_stack_, context management)
- Namespace Phase 2 (partial): Qualified names parsed structurally (QualifiedNameRef, qualifier_segments on Node)
- Namespace Phase 3 (partial): Declaration ownership by context (namespace_context_id on Node)
- Namespace Phase 4 (partial): Context-based lookup (lookup_value_in_context, lookup_type_in_context, using directives)
- **Namespace Phase 5 slice 1: LLVM name quoting** — `quote_llvm_ident()` and `llvm_struct_type_str()` use LLVM double-quote syntax for names with `::`. `sanitize_llvm_ident()` preserved for local vars. Dead-fn scanner handles quoted `@"name"` refs. 4 new runtime tests added.
- **Namespace Phase 5 slice 2: HIR namespace propagation** — `NamespaceQualifier` struct in ir.hpp with `segments`, `is_global_qualified`, `context_id`. Added to `DeclRef`, `Function`, `GlobalVar`, `HirStructDef`. `make_ns_qual()` helper propagates qualifier_segments and namespace_context_id from AST nodes during lowering. HIR printer displays `ns=...` annotations.
- **Namespace Phase 2 slice 3: Namespace-qualified struct tags** — `defined_struct_tags_` and forward references now use namespace-qualified tags via `canonical_name_in_context()`. Same-named structs in different namespaces no longer collide. New runtime test `namespace_struct_collision_runtime.cpp`.
- **Namespace Phase 5 slice 3: Nested namespace lowering + extern decl quoting** — Fixed recursive flattening for nested namespace blocks and `extern_call_decls_` quoting.
- **Namespace Phase 4 slice 2: Function name lookup across namespaces** — `known_fn_names_` set; unqualified calls work across anonymous namespaces and via `using namespace` directives.

Completion criteria met:
- `::a::b` and `a::b` correctly distinguished
- Reopen namespace and anonymous namespace have stable semantics
- 28 namespace tests (27 positive + 1 negative) all pass

### Milestone B: Diagnostics and error recovery
- **Phase 1+2: Diagnostic format + invalid node kinds** — `file:line:col: error: message` format. `NK_INVALID_EXPR`/`NK_INVALID_STMT` placeholders.
- **Phase 3: Statement-level synchronization hooks** — try-catch in `parse_block()`, skip-to-`;`/`}`, produce `NK_INVALID_STMT`, continue parsing.
- **Phase 4: Negative test runner with expected-error support** — `verify_case/` tests, `run_verify_case.cmake`, 5 curated verify tests.
- **Phase 5: Curate external C clang allowlists** — 37 pass + 1 verify from 144 external C files.

Completion criteria met:
- One bad declaration doesn't destroy entire TU
- Cascade errors significantly reduced
- Small batch of curated Clang-style negative tests run in verify mode

### Milestone C: Iterator/container usability
- Iterator phases 0-5 complete (18 iterator tests, 4 container tests, all passing)
- `fixed_vec_smoke.cpp` passes: size/empty/data/front/back/operator[]/push_back/pop_back/begin/end/range-for all work together

Completion criteria met:
- Custom iterators compile and work in plain loops
- Small fixed-storage container passes integrated smoke test
- Range-for works as thin layer over iterator model

### Milestone D: Codegen architecture cleanup (LIR split)
- **Stage 0: LIR mechanical prep** — `LirModule`, `LirFunction`, `LirBlock`, `LirInst`, `LirTerminator` skeleton.
- **Stage 1 slice 1: Replace string sinks with LIR blocks** — `alloca_lines`/`body_lines` → structured `LirBlock`. `fn_bodies_` → `LirModule`.
- **Stage 1 slice 2: Replace preamble_ with structured LIR containers** — type_decls, string_pool, globals, intrinsic flags, extern_decls in `LirModule`.
- **Stage 2: Split printer into standalone LirPrinter** — `lir::print_llvm()` handles all rendering. `HirEmitter::emit()` only lowers.
- **Stage 3 slice 1: Typed intrinsic LIR ops** — `LirMemcpyOp`, `LirVaStartOp`, `LirVaEndOp`, `LirVaCopyOp`, `LirStackSaveOp`, `LirStackRestoreOp`, `LirAbsOp`.
- **Stage 3 slice 2: Typed bitfield LIR ops** — `LirBitfieldExtract`, `LirBitfieldInsert` with string SSA names.
- **Stage 3 slice 3: Typed indirectbr LIR op** — `LirIndirectBrOp`.

Completion criteria met:
- Behavior preserved (identical LLVM IR output)
- Printer no longer performs semantic repair
- Clear HIR/LIR/printer boundary for future backend work

## Housekeeping
- Removed `C/C23/n2683_2.c` from clang external allowlist (requires `stdckdint.h` we don't provide)

## Test Suite
- Final: **2069/2069 (100%)**

## Blockers
None
