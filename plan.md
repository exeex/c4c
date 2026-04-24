# HIR Agent Index Header Hierarchy Runbook

Status: Active
Source Idea: ideas/open/93_hir_agent_index_header_hierarchy.md

## Purpose

Optimize `src/frontend/hir` for agent-oriented editing by making directories
communicate semantic boundaries and making headers serve as index entries for
those boundaries.

Goal: make top-level HIR headers public by default and nested `impl/` headers
private or subdomain-specific by default, without changing HIR behavior.

## Core Rule

`directory = semantic boundary`; `.hpp = index entry for that boundary`.

## Read First

- `ideas/open/93_hir_agent_index_header_hierarchy.md`
- `src/frontend/hir/hir.hpp`
- `src/frontend/hir/hir_ir.hpp`
- existing HIR private headers and split implementation files under
  `src/frontend/hir/`

## Current Targets

- Public facade and IR contract:
  - `src/frontend/hir/hir.hpp`
  - `src/frontend/hir/hir_ir.hpp`
- Current implementation headers to re-boundary:
  - `src/frontend/hir/hir_lowering.hpp`
  - `src/frontend/hir/hir_lowerer_internal.hpp`
  - `src/frontend/hir/compile_time_engine.hpp`
  - `src/frontend/hir/inline_expand.hpp`
  - `src/frontend/hir/hir_printer.hpp`
- Split HIR implementation files under `src/frontend/hir/`, especially
  expression, statement, template/dependent, compile-time, inline expansion,
  and inspection/printer files.

## Non-Goals

- Do not change HIR semantics.
- Do not change template or consteval behavior.
- Do not change codegen-facing HIR output.
- Do not downgrade testcase expectations.
- Do not split `hir_ir.hpp` merely for C++ interface purity.
- Do not create one header per HIR `.cpp`.
- Do not do broad `.cpp` movement before public/private header boundaries are
  clean.
- Do not mix this work with parser visible-name result semantics or unrelated
  HIR representation redesign.

## Working Model

The preferred visibility shape is:

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
  follow-up transforms
```

The first structural win is honest header visibility and useful directory
indexes. Moving every `.cpp` file is not required for the first slices.

## Execution Rules

- Keep each step behavior-preserving.
- Prefer include-path and build-system updates that follow from the chosen
  boundary; do not broaden the slice into unrelated cleanup.
- When a semantic issue appears during movement, record it separately unless
  fixing it is required for the structural slice to compile.
- After each code-changing step, run at least:
  - `cmake --build build -j --target c4c_frontend c4cll`
  - `ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$'`
- Also run parser tests if public AST/parser-facing includes change:
  - `ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`
- Escalate validation if a slice changes codegen-facing HIR declarations,
  template materialization, compile-time normalization, or LIR/LLVM include
  paths.

## Step 1: Confirm Public HIR Surface

Goal: decide which existing top-level HIR declarations are intentionally public
before moving implementation indexes.

Primary targets:
- `src/frontend/hir/hir.hpp`
- `src/frontend/hir/hir_ir.hpp`
- `src/frontend/hir/hir_printer.hpp`

Actions:
- Inspect callers of `hir.hpp`, `hir_ir.hpp`, `format_hir(...)`, and
  `format_summary(...)`.
- Keep `hir.hpp` as the public facade for HIR pipeline entry points, options,
  result types, and intentionally public summary helpers.
- Keep `hir_ir.hpp` as the centralized HIR data-model contract.
- Decide whether printer/summary declarations belong in the facade or in an
  inspection boundary; make only the minimal movement needed to remove
  ambiguity.

Completion check:
- Public HIR surfaces are identified.
- No private lowering machinery is newly exposed through top-level HIR headers.
- Build proof is recorded in `todo.md`.

## Step 2: Create Private HIR Implementation Boundary

Goal: make HIR implementation files include private indexes instead of relying
on top-level implementation headers.

Primary targets:
- `src/frontend/hir/impl/hir_impl.hpp`
- `src/frontend/hir/impl/lowerer.hpp`
- declarations currently in `src/frontend/hir/hir_lowering.hpp`
- declarations currently in `src/frontend/hir/hir_lowerer_internal.hpp`

Actions:
- Introduce `impl/hir_impl.hpp` as the private HIR implementation index.
- Move or forward declarations currently owned by `hir_lowering.hpp` through
  `impl/hir_impl.hpp`.
- Introduce `impl/lowerer.hpp` for the private `Lowerer` engine declarations
  currently owned by `hir_lowerer_internal.hpp`.
- Update HIR implementation `.cpp` files to include the private indexes.
- Remove compatibility includes only when include sites have been retargeted.

Completion check:
- Agents editing HIR internals can start from `impl/hir_impl.hpp` and
  `impl/lowerer.hpp`.
- Top-level HIR headers represent public surfaces by default.
- `c4c_frontend`, `c4cll`, and focused HIR tests build/pass.

## Step 3: Add Expression And Statement Subdomain Indexes

Goal: make expression and statement lowering navigable through subdomain
indexes after the private HIR boundary is clean.

Primary targets:
- `src/frontend/hir/impl/expr/expr.hpp`
- `src/frontend/hir/impl/stmt/stmt.hpp`
- expression and statement lowering implementation files

Actions:
- Introduce `impl/expr/expr.hpp` for expression dispatch and related helper
  declarations.
- Introduce `impl/stmt/stmt.hpp` for statement dispatch and related helper
  declarations.
- Retarget expression and statement implementation includes to the subdomain
  indexes.
- Move `.cpp` files only when the include boundary is already clean and the
  movement is a coherent slice.

Completion check:
- Agents editing expression or statement lowering have one obvious subdomain
  header to open.
- No one-header-per-`.cpp` pattern is introduced.
- Focused HIR proof is recorded in `todo.md`.

## Step 4: Split Template/Dependent Lowering Boundary

Goal: give template/dependent HIR lowering a single private directory index if
the family remains large after the core boundary work.

Primary target:
- `src/frontend/hir/impl/templates/templates.hpp`

Actions:
- Move template-family implementation files under `impl/templates/` only after
  the HIR implementation boundary is clean.
- Add one `templates.hpp` index for deduction, deferred NTTP, materialization,
  struct instantiation, type resolution, and value argument helpers.
- Avoid one-off headers for deduction, materialization, NTTP, or member typedef
  work unless they become real directories.

Completion check:
- Template/dependent lowering has a directory-level private index when moved.
- Template and consteval behavior is unchanged.
- HIR tests pass; escalate validation for compile-time/materialization impact.

## Step 5: Move Compile-Time And Follow-Up Transform Boundary

Goal: make compile-time normalization, materialization state, and tightly
coupled HIR-local follow-up transforms explicit.

Primary target:
- `src/frontend/hir/impl/compile_time/compile_time.hpp`

Actions:
- Introduce `impl/compile_time/compile_time.hpp`.
- Move compile-time engine declarations there once callers no longer depend on
  top-level accidental visibility.
- Move `inline_expand.hpp` and its implementation into this area if inline
  expansion remains coupled to compile-time/materialized HIR state.
- If inline expansion should become a generic transform subsystem instead,
  record that as separate scope rather than forcing it into this step.

Completion check:
- Compile-time engine and inline expansion have an explicit boundary decision.
- Compile-time normalization and materialization behavior is unchanged.
- Broader validation is run if codegen-facing or compile-time paths are touched.

## Step 6: Clarify Inspection Support

Goal: make HIR printer/debug support either intentionally public or private
inspection support.

Primary target:
- `src/frontend/hir/impl/inspect/inspect.hpp`

Actions:
- Keep `format_summary(...)` in the public facade if external callers use it as
  a normal API.
- Move printer/debug declarations under `impl/inspect/` if they are
  implementation/debug-only.
- Keep inspection movement separate from semantic printer behavior changes.

Completion check:
- HIR inspection declarations are no longer split ambiguously between facade
  and private support.
- External callers retain intended public formatting/summary APIs.
- Focused HIR proof is recorded in `todo.md`.

## Step 7: Refresh HIR README

Goal: align the directory map with the actual HIR layout after structural
slices land.

Primary target:
- `src/frontend/hir/README.md`

Actions:
- Update the README to describe public facade, IR contract, private
  implementation indexes, and any subdomain directories that now exist.
- Keep the README as a high-level map, not a substitute for the index headers.

Completion check:
- README matches the final file names and boundaries.
- It does not document aspirational files that were not created.

## Step 8: Final Structural Validation

Goal: prove the HIR header hierarchy work is complete for this initiative.

Actions:
- Confirm acceptance criteria from the source idea are satisfied or explicitly
  record any deferred follow-up as a separate open idea.
- Run the required build and focused HIR tests.
- Escalate to parser tests, broader CTest, or full scan if include movement
  crossed public AST/parser-facing, codegen-facing, template materialization,
  compile-time normalization, or LIR/LLVM boundaries.

Completion check:
- `c4c_frontend`, `c4cll`, and HIR-related tests build/pass.
- The active runbook and source idea can be considered for closure by the plan
  owner only after source-idea acceptance criteria are actually satisfied.
