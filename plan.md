# `ast_to_hir.cpp` Compression Refactor Plan

## Plan Metadata

- Status: Active
- Source Idea: [ideas/open/ast_to_hir_compression_plan.md](/workspaces/c4c/ideas/open/ast_to_hir_compression_plan.md)

## Purpose

Execution runbook for a behavior-preserving compression refactor of
[src/frontend/hir/ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp).

## One-Sentence Goal

Compress `ast_to_hir.cpp` by extracting repeated lowering and template/type
resolution mechanics into named helpers while preserving lowering order,
template seeding, fixpoint behavior, and emitted HIR semantics.

## Core Rule

Treat this as extraction-first refactoring:

1. preserve lowering order exactly
2. keep compile-time seeding and realization behavior unchanged
3. move one mechanism family at a time behind named helpers
4. validate each slice with the narrowest HIR/lowering coverage first, then the
   full suite

Do not turn this into a semantic cleanup pass.

## Read First

1. [src/frontend/hir/ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp)
2. [tests/cpp/internal/InternalTests.cmake](/workspaces/c4c/tests/cpp/internal/InternalTests.cmake)
3. [ideas/open/ast_to_hir_compression_plan.md](/workspaces/c4c/ideas/open/ast_to_hir_compression_plan.md)

Focus first on:

1. `Lowerer::lower_initial_program`
2. pending template-struct resolution helpers
3. function/method lowering setup
4. embedded parser helpers
5. layout helper cluster

## Scope

- [src/frontend/hir/ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp)
- HIR dump coverage under [tests/cpp/internal](/workspaces/c4c/tests/cpp/internal)

## Non-Goals

- no lowering-order changes
- no broad template-instantiation policy changes
- no cross-file reorganization in one patch
- no opportunistic frontend/codegen fixes unrelated to the active compression
  slice

## Execution Rules

1. Keep each patch to one compression cluster.
2. Add or update a narrow test before implementation.
3. Prefer coordinator/helper extraction over rewritten logic.
4. Compare behavior against existing HIR dump output when practical.
5. Run targeted HIR tests before the full `ctest` suite.

## Step 1. Compress Initial Program Orchestration

Goal:

- turn `Lowerer::lower_initial_program(...)` into a readable phase
  coordinator

Primary target:

- `Lowerer::lower_initial_program(...)`

Do this:

1. extract named helpers for top-level item flattening
2. extract phase helpers for weak-symbol collection, type-definition
   collection, consteval/template registration, template seed realization, and
   metadata finalization
3. keep every phase in the current order
4. keep the ref-overload, out-of-class method attachment, and Phase 2 lowering
   behavior unchanged

Completion check:

- `lower_initial_program(...)` reads as a coordinator over named phases and HIR
  regression coverage stays green

## Step 2. Compress Pending Template Struct Resolution

Goal:

- unify repeated pending-template and member-typedef resolution sequences

Primary targets:

- `resolve_pending_tpl_struct_if_needed`
- `resolve_pending_tpl_struct`
- `instantiate_deferred_template_type`
- `instantiate_template_struct_body`

Completion check:

- repeated template-struct resolution chains are centralized behind shared
  helpers without semantic drift

## Step 3. Compress Function/Method Lowering Setup

Goal:

- share repeated function and method lowering setup code

Primary targets:

- `lower_function`
- `lower_struct_method`

Completion check:

- setup and signature-binding paths are staged through shared helpers with
  unchanged lowering behavior

## Step 4. Isolate Embedded Parser Helpers

Goal:

- isolate the tiny text/parser subsystem from HIR lowering orchestration

Completion check:

- embedded parsing entrypoints are grouped behind a narrow helper surface

## Step 5. Isolate Layout Helpers

Goal:

- isolate type/layout-only math from lowering orchestration

Completion check:

- layout helpers are clearly separated from lowering coordination logic
