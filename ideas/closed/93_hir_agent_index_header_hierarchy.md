# HIR Agent Index Header Hierarchy

Status: Closed
Created: 2026-04-24
Last Updated: 2026-04-24
Parent Ideas:
- [92_parser_agent_index_header_hierarchy.md](/workspaces/c4c/ideas/closed/92_parser_agent_index_header_hierarchy.md)

Reopened: 2026-04-24
Closed: 2026-04-24

## Completion Notes

Closed after the active runbook completed the HIR agent-index migration:

- `hir.hpp` and `hir_ir.hpp` remain the public HIR facade and IR contract.
- Private HIR implementation indexes are under `src/frontend/hir/impl/`.
- Expression, statement, template, compile-time, and inspection implementation
  files live under their semantic `impl/` subdirectories with redundant
  filename prefixes removed.
- Top-level remaining HIR `.cpp` files are root/facade/pipeline implementation
  for this plan's scope.
- Stale old implementation filename references were checked outside lifecycle
  and archive paths.
- Focused `^cpp_hir` validation, the public include probe, and full CTest
  validation passed before closure.

## Reopen Note

The earlier structural pass moved private HIR headers under `impl/`, but it did
not move the implementation `.cpp` files into the semantic directories. That is
not enough for the intended agent workflow. The source tree itself should show
where an agent should work, and moving files should also remove repeated file
name prefixes such as `hir_expr_`, `hir_stmt_`, `hir_templates_`, and
`compile_time_engine`.

The required shape is:

```text
src/frontend/hir/
  hir.hpp
  hir_ir.hpp
  hir.cpp

  impl/
    hir_impl.hpp
    lowerer.hpp

    expr/
      expr.hpp
      expr.cpp
      builtin.cpp
      call.cpp
      object.cpp
      operator.cpp
      scalar_control.cpp

    stmt/
      stmt.hpp
      stmt.cpp
      control_flow.cpp
      decl.cpp
      range_for.cpp
      switch.cpp

    templates/
      templates.hpp
      templates.cpp
      deduction.cpp
      deferred_nttp.cpp
      global.cpp
      materialization.cpp
      member_typedef.cpp
      struct_instantiation.cpp
      type_resolution.cpp
      value_args.cpp

    compile_time/
      compile_time.hpp
      engine.cpp
      inline_expand.cpp
```

The top-level `hir_*.cpp` implementation prefix is not the desired final agent
surface. Once a file lives under a semantic directory, the directory should
carry that context and the filename should be short.

## Goal

Optimize `src/frontend/hir` for LLM-agent programming by treating headers as
semantic index surfaces for HIR's real subdomains: public facade, IR contract,
private HIR implementation, expression lowering, statement lowering, template
lowering, and compile-time/materialization work.

HIR is currently the frontend area most in need of this discipline because it
sits between parser, sema, template/dependent semantic resolution, inline
expansion, and backend-facing codegen consumers.

## Why This Idea Exists

`src/frontend/hir` has many split implementation files in one flat directory:

- expression lowering files
- statement lowering files
- type/layout/init lowering files
- template instantiation and materialization files
- compile-time engine files
- inline expansion files
- printer/debug files
- public facade and IR contract files

The current headers mix several roles at the same path depth:

- `hir.hpp` is the public facade
- `hir_ir.hpp` is the public HIR data-model contract
- `hir_lowering.hpp` is a shared lowering surface, but lives beside public
  headers
- `hir_lowerer_internal.hpp` is implementation-only, but also lives beside
  public headers
- `compile_time_engine.hpp`, `inline_expand.hpp`, and `hir_printer.hpp` each
  expose pass-specific surfaces without a directory hierarchy that tells an
  agent which are public contracts and which are local implementation indexes

This causes agents to spend context discovering boundaries that the file tree
should communicate directly.

## Design Direction

Use this rule:

```text
directory = semantic boundary
.hpp      = index entry for that boundary
```

For HIR, path depth should communicate visibility:

```text
src/frontend/hir/*.hpp
  public HIR facade and frontend/backend-facing HIR contract

src/frontend/hir/impl/*.hpp
  private to HIR implementation files

src/frontend/hir/impl/expr/*.hpp
  private to expression lowering internals

src/frontend/hir/impl/stmt/*.hpp
  private to statement lowering internals

src/frontend/hir/impl/templates/*.hpp
  private to template/dependent lowering internals

src/frontend/hir/impl/compile_time/*.hpp
  private to compile-time normalization, materialization, and HIR-local
  follow-up transforms such as inline expansion
```

Top-level HIR headers should be public by default. Nested `impl/` headers
should be private to HIR implementation unless explicitly promoted by a
top-level facade.

## Target Shape

Preferred long-term layout:

```text
src/frontend/hir/
  hir.hpp
  hir_ir.hpp
  hir.cpp

  impl/
    hir_impl.hpp
    lowerer.hpp

    expr/
      expr.hpp
      expr.cpp
      builtin.cpp
      call.cpp
      object.cpp
      operator.cpp
      scalar_control.cpp

    stmt/
      stmt.hpp
      stmt.cpp
      control_flow.cpp
      decl.cpp
      range_for.cpp
      switch.cpp

    templates/
      templates.hpp
      templates.cpp
      deduction.cpp
      deferred_nttp.cpp
      global.cpp
      materialization.cpp
      member_typedef.cpp
      struct_instantiation.cpp
      type_resolution.cpp
      value_args.cpp

    compile_time/
      compile_time.hpp
      engine.cpp
      inline_expand.cpp
```

The key requirement is not merely adding private index headers. The
implementation `.cpp` files should move into the semantic `impl/` subdirectory
that owns them, and their filenames should drop duplicated prefixes that the
directory path already communicates.

Files that do not clearly belong to `expr/`, `stmt/`, `templates/`, or
`compile_time/` may stay at the HIR top level only if they are genuinely part
of the public facade or root HIR orchestration. Otherwise they should move
under `impl/` with similarly shortened names. A possible root implementation
shape is:

```text
src/frontend/hir/impl/
  build.cpp
  functions.cpp
  core.cpp
  types.cpp
```

The first reopened slice may move one family at a time, but the acceptance
target is the file-tree migration, not only header visibility.

## Public Header Roles

### `hir.hpp`

The public facade should remain small:

- HIR pipeline entry points such as `build_hir(...)`
- high-level summary helpers if they are intentionally public
- public options/result types needed by callers

It should not be the place an agent reads to understand every lowering helper.

### `hir_ir.hpp`

The public HIR data-model contract should remain centralized:

- Module, Function, Block, Stmt, Expr, Type, ID, and metadata structures
- data structures consumed by sema, LIR/codegen, tests, and inspection tooling
- small inline helpers tightly coupled to the HIR contract

Do not split HIR IR declarations just because individual implementation files
touch them. Split only if a real public semantic subdirectory emerges.

## Private / Subdomain Header Roles

### `impl/hir_impl.hpp`

Private HIR implementation index:

- shared lowering entry points for initial HIR construction
- source/span/string utilities used by lowering implementation files
- shared lowering result carriers
- map/infer/normalize helpers used across expression, statement, type, and
  function lowering

This replaces the current top-level `hir_lowering.hpp` role.

### `impl/lowerer.hpp`

Private `Lowerer` engine index:

- implementation-only `Lowerer` class
- cross-file method declarations grouped by lowering phase
- private parser/sema/HIR helper dependencies
- navigation map for lowering implementation files

This replaces the current top-level `hir_lowerer_internal.hpp` role.

### `impl/expr/expr.hpp`

Expression lowering index:

- expression dispatch
- call/member/builtin/operator/object expression helpers
- scalar/control-expression lowering helpers that belong with expression
  semantics

Avoid one header per expression `.cpp`; use the directory index for the
expression lowering family.

### `impl/stmt/stmt.hpp`

Statement lowering index:

- statement dispatch
- declaration statements
- control-flow statements
- range-for and switch/case/default lowering helpers

Avoid one header per statement `.cpp`; use the directory index for the
statement lowering family.

### `impl/templates/templates.hpp`

Template/dependent lowering index if the template family remains large:

- template argument deduction and materialization helpers
- deferred NTTP and pending template-type work
- template struct instantiation helpers
- value/type argument resolution helpers

Avoid creating one header per template `.cpp`. Use this as the directory index
for the whole template lowering subdomain.

### `impl/compile_time/compile_time.hpp`

Compile-time normalization engine boundary:

- deferred compile-time work queues and result carriers
- template/consteval fixpoint driver API
- materialization-state APIs needed by the HIR facade
- HIR-local follow-up transforms that depend on compile-time/materialized
  state, including inline expansion unless it grows into a broader transform
  subsystem

### `impl/inspect/inspect.hpp`

Private debug/inspection index if printer/debug support is not public facade:

- HIR formatter/dumper declarations
- summary helpers
- debug-only inspection utilities

This can replace thin top-level printer-only headers when those declarations
are not part of the core public facade.

## Refactoring Steps

1. Confirm the public HIR surface.
   - Keep `hir.hpp` as the facade.
   - Keep `hir_ir.hpp` as the HIR data-model contract.
   - Decide whether `format_hir(...)` and `format_summary(...)` are facade
     APIs or inspection APIs; do not leave them split ambiguously.

2. Create the private HIR implementation boundary.
   - Introduce `src/frontend/hir/impl/hir_impl.hpp`.
   - Move or forward through the declarations currently in `hir_lowering.hpp`.
   - Introduce `src/frontend/hir/impl/lowerer.hpp` for the declarations
     currently in `hir_lowerer_internal.hpp`.
   - Update HIR implementation `.cpp` files to include the private
     implementation indexes.

3. Move expression and statement families behind subdomain indexes.
   - Introduce `src/frontend/hir/impl/expr/expr.hpp`.
   - Introduce `src/frontend/hir/impl/stmt/stmt.hpp`.
   - Move expression and statement `.cpp` families under
     `src/frontend/hir/impl/expr/` and `src/frontend/hir/impl/stmt/`.
   - Rename moved files to remove duplicated prefixes:
     `hir_expr.cpp` -> `impl/expr/expr.cpp`,
     `hir_expr_builtin.cpp` -> `impl/expr/builtin.cpp`,
     `hir_stmt.cpp` -> `impl/stmt/stmt.cpp`, and so on.

4. Split template lowering only after the HIR implementation boundary is clean.
   - Move template-family implementation files under
     `src/frontend/hir/impl/templates/`.
   - Add one `templates.hpp` index for the subdomain.
   - Rename moved files to remove the duplicated `hir_templates_` prefix:
     `hir_templates.cpp` -> `impl/templates/templates.cpp`,
     `hir_templates_deduction.cpp` -> `impl/templates/deduction.cpp`, and so
     on.
   - Do not create one-off headers for deduction, materialization, NTTP, or
     member typedef work unless those become real directories.

5. Move compile-time and HIR-local follow-up work behind one index.
   - Introduce `src/frontend/hir/impl/compile_time/compile_time.hpp`.
   - Move compile-time engine declarations there after callers are no longer
     depending on top-level accidental visibility.
   - Move `inline_expand.hpp/cpp` into this area if it remains coupled to
     compile-time/materialized HIR state instead of becoming a generic pass
     subsystem.
   - Rename `compile_time_engine.cpp` to `impl/compile_time/engine.cpp`.
   - Move `inline_expand.cpp` to `impl/compile_time/inline_expand.cpp`.

6. Move inspection support only when its public role is clear.
   - Keep `format_summary(...)` in the public facade if external callers use
     it as a normal API.
   - Move printer/debug declarations under `impl/inspect/` if they are
     implementation/debug-only.

7. Refresh `src/frontend/hir/README.md`.
   - Align the README with actual file names and directory boundaries.
   - Keep it as a high-level map, not a substitute for the directory index
     headers.

8. Preserve behavior.
   - This initiative is structural.
   - Do not change HIR semantics, template behavior, compile-time
     normalization behavior, codegen-facing HIR output, or testcase
     expectations as part of the layout refactor.
   - Semantic cleanup discovered during movement should become a separate
     idea unless required to keep the structural slice compiling.

## Acceptance Criteria

- External callers can include `src/frontend/hir/hir.hpp` for the facade and
  `src/frontend/hir/hir_ir.hpp` for the data-model contract without pulling in
  full private lowering machinery.
- Agents editing AST-to-HIR lowering can open
  `src/frontend/hir/impl/hir_impl.hpp` and
  `src/frontend/hir/impl/lowerer.hpp` as the implementation indexes.
- Agents editing expression or statement lowering can open
  `src/frontend/hir/impl/expr/expr.hpp` or
  `src/frontend/hir/impl/stmt/stmt.hpp` as subdomain indexes.
- Expression `.cpp` implementation files live under
  `src/frontend/hir/impl/expr/` with redundant `hir_expr_` prefixes removed.
- Statement `.cpp` implementation files live under
  `src/frontend/hir/impl/stmt/` with redundant `hir_stmt_` prefixes removed.
- Agents editing template/dependent lowering can open
  `src/frontend/hir/impl/templates/templates.hpp` as the subdomain index
  if the template family is moved.
- Template `.cpp` implementation files live under
  `src/frontend/hir/impl/templates/` with redundant `hir_templates_` prefixes
  removed.
- Compile-time engine and inline expansion have an explicit
  `impl/compile_time/` directory-level index instead of floating at top-level,
  unless inline expansion is deliberately promoted into a separate transform
  subsystem later.
- Compile-time `.cpp` implementation files live under
  `src/frontend/hir/impl/compile_time/`, including `engine.cpp` and
  `inline_expand.cpp` when inline expansion remains compile-time/materialized
  HIR follow-up work.
- Top-level HIR headers represent public surfaces by default.
- Nested `impl/` HIR headers represent private or narrower subdomain surfaces by
  default.
- The number of thin single-purpose top-level HIR headers decreases or stays
  flat.
- `c4c_frontend`, `c4cll`, and HIR-related tests build and pass after each
  structural slice.

## Validation

At minimum:

- `cmake --build build -j --target c4c_frontend c4cll`
- `ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$'`

Also run parser tests if public AST/parser-facing includes change:

- `ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`

Escalate validation if the slice changes codegen-facing HIR declarations,
template materialization, compile-time normalization, or LIR/LLVM include
paths.

## Non-Goals

- no traditional separated `include/` tree
- no one-header-per-HIR-`.cpp` convention
- no HIR semantic changes
- no template or consteval behavior changes
- no expectation downgrades as proof
- no splitting `hir_ir.hpp` merely for C++ interface purity
- no broad `.cpp` movement before the public/private header boundaries are
  clean
- no mixing this structural work with parser visible-name result semantics or
  unrelated HIR representation redesign

## Relationship To Nearby Work

This idea is related in style to:

- [92_parser_agent_index_header_hierarchy.md](/workspaces/c4c/ideas/closed/92_parser_agent_index_header_hierarchy.md)

Parser and HIR header hierarchy work should stay separate because their public
contracts are different:

- parser owns syntax and parser-private state
- HIR owns the typed frontend IR contract and second semantic lowering layer

If both are active candidates, prefer not to change parser and HIR include
hierarchies in the same commit unless the change is limited to build-system or
include-path fallout required by one coherent slice.
