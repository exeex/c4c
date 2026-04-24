# HIR Agent Index Header Hierarchy Runbook

Status: Active
Source Idea: ideas/open/93_hir_agent_index_header_hierarchy.md

## Purpose

Make `src/frontend/hir` readable as an agent-facing semantic file tree:
public facade and IR contract at the top level, private implementation indexes
under `impl/`, and implementation `.cpp` files under the subdomain directory
that owns them.

## Goal

Move HIR implementation files into semantic directories with short names while
preserving HIR behavior and public facade contracts.

## Core Rule

This is a structural refactor only. Do not change HIR semantics, template or
consteval behavior, codegen-facing HIR output, or testcase expectations.

## Read First

- `ideas/open/93_hir_agent_index_header_hierarchy.md`
- `src/frontend/hir/README.md`
- `src/frontend/hir/hir.hpp`
- `src/frontend/hir/hir_ir.hpp`
- `src/frontend/hir/impl/hir_impl.hpp`
- `src/frontend/hir/impl/lowerer.hpp`

## Current Scope

Primary directory:

- `src/frontend/hir/`

Target private indexes and implementation directories:

- `src/frontend/hir/impl/hir_impl.hpp`
- `src/frontend/hir/impl/lowerer.hpp`
- `src/frontend/hir/impl/expr/expr.hpp`
- `src/frontend/hir/impl/stmt/stmt.hpp`
- `src/frontend/hir/impl/templates/templates.hpp`
- `src/frontend/hir/impl/compile_time/compile_time.hpp`
- `src/frontend/hir/impl/inspect/inspect.hpp` when inspection is private

## Non-Goals

- Do not introduce a traditional separated `include/` tree.
- Do not create one header per HIR `.cpp` file.
- Do not split `hir_ir.hpp` merely for interface purity.
- Do not mix this work with parser visible-name semantics or unrelated HIR IR
  redesign.
- Do not downgrade tests or rewrite expectations as proof.

## Working Model

- Top-level `src/frontend/hir/*.hpp` files are public by default.
- Nested `src/frontend/hir/impl/**/*.hpp` files are private or subdomain
  indexes by default.
- Directory names should carry semantic context, so moved `.cpp` files should
  drop redundant prefixes such as `hir_expr_`, `hir_stmt_`,
  `hir_templates_`, and `compile_time_engine`.
- Keep code movement in buildable packets and let recursive source discovery
  pick up moved `.cpp` files unless evidence shows an explicit build-system
  update is needed.

## Execution Rules

- Use `git mv` for file moves when possible.
- Keep each packet behavior-preserving.
- Update include paths immediately after moving a file or header.
- Prefer directory index headers over new thin one-off headers.
- If a semantic cleanup is discovered that is not required for compilation,
  record it as a separate idea instead of folding it into this plan.
- For code-changing steps, run:
  `cmake --build build -j --target c4c_frontend c4cll`
  and
  `ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$'`.
- Also run parser tests if public AST/parser-facing includes change:
  `ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`.
- Escalate to broader validation when a packet changes codegen-facing HIR
  declarations, template materialization, compile-time normalization, or
  LIR/LLVM include paths.

## Steps

### Step 1. Inventory HIR Layout And Build References

Goal: establish the exact current file map and source-discovery behavior before
moving files.

Concrete actions:

- List current `src/frontend/hir` files and classify them as public facade,
  public IR contract, private root implementation, expression, statement,
  template, compile-time, or inspection support.
- Check CMake/source discovery for HIR files and note whether explicit source
  lists need updates after moves.
- Record the intended move map in `todo.md`.

Completion check:

- `todo.md` names the current HIR file families, target directories, and any
  build-system touch points.

### Step 2. Clean Root Public And Private HIR Boundaries

Goal: ensure top-level public headers and private root implementation indexes
have clear roles before broad `.cpp` movement.

Primary targets:

- `src/frontend/hir/hir.hpp`
- `src/frontend/hir/hir_ir.hpp`
- `src/frontend/hir/impl/hir_impl.hpp`
- `src/frontend/hir/impl/lowerer.hpp`
- legacy top-level private headers such as `hir_lowering.hpp`,
  `hir_lowerer_internal.hpp`, `compile_time_engine.hpp`, `inline_expand.hpp`,
  and `hir_printer.hpp` if still present

Concrete actions:

- Keep `hir.hpp` as the public pipeline facade.
- Keep `hir_ir.hpp` as the public HIR data-model contract.
- Ensure private lowering declarations are reachable through
  `impl/hir_impl.hpp` and `impl/lowerer.hpp`.
- Decide whether `format_hir(...)` and `format_summary(...)` remain public
  facade APIs or move behind inspection.
- Remove accidental top-level private header exposure only when call sites have
  been updated.

Completion check:

- Public callers can include `hir.hpp` and `hir_ir.hpp` without depending on
  private lowering machinery.
- HIR implementation files use private implementation indexes for lowering
  internals.
- Focused HIR proof passes.

### Step 3. Move Expression And Statement Implementation Families

Goal: move expression and statement lowering files under their semantic
subdomain directories and shorten redundant filenames.

Primary targets:

- `src/frontend/hir/impl/expr/expr.hpp`
- `src/frontend/hir/impl/stmt/stmt.hpp`
- `src/frontend/hir/hir_expr*.cpp`
- `src/frontend/hir/hir_stmt*.cpp`

Concrete actions:

- Move expression implementation files under `src/frontend/hir/impl/expr/`.
- Rename moved expression files to short names such as `expr.cpp`,
  `builtin.cpp`, `call.cpp`, `object.cpp`, `operator.cpp`, and
  `scalar_control.cpp`.
- Move statement implementation files under `src/frontend/hir/impl/stmt/`.
- Rename moved statement files to short names such as `stmt.cpp`,
  `control_flow.cpp`, `decl.cpp`, `range_for.cpp`, and `switch.cpp`.
- Update includes and stale filename references.

Completion check:

- No moved top-level `hir_expr*.cpp` or `hir_stmt*.cpp` files remain.
- Expression and statement implementation files live under their target
  directories with prefix-free names.
- Focused HIR proof passes.

### Step 4. Move Template Lowering Family

Goal: place template/dependent lowering implementation under one template
subdomain index and directory.

Primary targets:

- `src/frontend/hir/impl/templates/templates.hpp`
- `src/frontend/hir/hir_templates*.cpp`

Concrete actions:

- Move template-family implementation files under
  `src/frontend/hir/impl/templates/`.
- Rename moved files to short names such as `templates.cpp`, `deduction.cpp`,
  `deferred_nttp.cpp`, `global.cpp`, `materialization.cpp`,
  `member_typedef.cpp`, `struct_instantiation.cpp`, `type_resolution.cpp`,
  and `value_args.cpp`.
- Keep one directory index header; do not create one-off headers for each
  template file.
- Update includes and stale filename references.

Completion check:

- No moved top-level `hir_templates*.cpp` files remain.
- Template implementation files live under `impl/templates/` with prefix-free
  names.
- Focused HIR proof passes, with broader validation if template materialization
  blast radius warrants it.

### Step 5. Move Compile-Time And HIR-Local Follow-Up Work

Goal: give compile-time normalization and coupled HIR-local follow-up transforms
an explicit private directory boundary.

Primary targets:

- `src/frontend/hir/impl/compile_time/compile_time.hpp`
- `src/frontend/hir/compile_time_engine.cpp`
- `src/frontend/hir/inline_expand.cpp`
- `src/frontend/hir/compile_time_engine.hpp`
- `src/frontend/hir/inline_expand.hpp`

Concrete actions:

- Move compile-time engine declarations behind
  `impl/compile_time/compile_time.hpp`.
- Move `compile_time_engine.cpp` to `impl/compile_time/engine.cpp`.
- Move `inline_expand.cpp` to `impl/compile_time/inline_expand.cpp` when it
  remains coupled to compile-time/materialized HIR state.
- Remove or demote top-level compile-time headers after call sites use the
  private index.
- Update includes and stale filename references.

Completion check:

- Compile-time implementation files live under `impl/compile_time/`.
- Top-level compile-time headers do not expose private HIR implementation
  machinery by accident.
- Focused HIR proof passes, with broader validation if compile-time behavior or
  LIR/LLVM paths are touched.

### Step 6. Resolve Inspection Surface And Refresh Documentation

Goal: make inspection/debug support and the HIR README match the actual tree.

Primary targets:

- `src/frontend/hir/impl/inspect/inspect.hpp`
- `src/frontend/hir/hir_printer.cpp`
- `src/frontend/hir/hir_printer.hpp`
- `src/frontend/hir/README.md`

Concrete actions:

- Keep formatting APIs in the public facade only when external callers use them
  as normal public APIs.
- Move private printer/debug declarations under `impl/inspect/` when they are
  implementation-only.
- Update README path maps, header roles, and agent navigation guidance to match
  the final layout.
- Search for stale old filename and header references outside lifecycle and
  archive paths.

Completion check:

- Inspection support has an explicit public or private role.
- README describes the actual file tree and index headers.
- No stale old filename references remain in live source, docs, tests, build
  files, or scripts.
- Focused HIR proof passes.

### Step 7. Final Acceptance Validation

Goal: prove the structural migration satisfies the source idea acceptance
criteria.

Concrete actions:

- Verify public facade includes still work for external callers:
  `src/frontend/hir/hir.hpp` and `src/frontend/hir/hir_ir.hpp`.
- Verify target implementation files live under `impl/expr/`, `impl/stmt/`,
  `impl/templates/`, and `impl/compile_time/` with redundant prefixes removed.
- Verify private implementation indexes are the agent entry points for HIR
  implementation work.
- Run focused build and HIR tests.
- Run broader or full validation if any prior step changed codegen-facing HIR
  declarations, template materialization, compile-time normalization, or
  LIR/LLVM include paths.

Completion check:

- Acceptance criteria from the source idea are met or any remaining scope is
  explicitly recorded for a follow-up idea.
- Required validation is green and recorded in `todo.md`.
