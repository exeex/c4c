# Parser Namespace Lookup Helper Extraction Runbook

Status: Active
Source Idea: ideas/open/01_parser_namespace_lookup_helper_extraction.md

## Purpose

Turn parser namespace lookup policy and HIR expression lowering subflows into
named helper seams so parser and HIR entrypoints read as coordinators rather
than mixed dispatch-plus-policy implementations.

## Goal

Preserve current frontend behavior while extracting repeated parser lookup and
HIR expression-lowering logic behind stable helper operations.

## Core Rule

Keep this slice behavior-preserving. Extract along semantic seams, do not mix
in backend work, broad data-structure swaps, or unrelated compile-time
experiments.

## Read First

- `ideas/open/01_parser_namespace_lookup_helper_extraction.md`
- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_core.cpp`
- `src/frontend/hir/hir_expr.cpp`

## Current Targets

- parser namespace-qualified name resolution
- parser visible value/type/concept lookup
- parser namespace context traversal and canonical-name synthesis
- HIR variable/member/call/template lowering subflows
- repeated ref/rvalue-ref argument lowering and temporary materialization logic

## Non-Goals

- backend refactors
- HIR or sema symbol-table redesign
- standalone container migrations
- broad compile-time investigation outside the helper seams in this idea

## Working Model

- parser helpers should centralize stack walk, alias handling, and canonical
  name construction without changing visibility rules or rollback behavior
- HIR helpers should leave `lower_expr(...)` as the entrypoint while moving
  heavy expression categories into named lowering helpers
- call lowering should become orchestration plus dedicated helpers for
  constructor, method, template, operator, and argument-materialization paths

## Execution Rules

- inspect existing call sites before extracting shared policy
- prefer small helper boundaries that map to stable semantic responsibilities
- keep helper ownership frontend-local for this slice
- move implementation into `.cpp` helpers when that improves clarity without
  forcing header churn
- validate parser lookup behavior and HIR lowering behavior after each major
  extraction step
- if new unrelated cleanup is discovered, record it back into `ideas/open/`
  instead of widening this plan

## Step 1: Baseline And Extraction Map

Goal: identify the concrete parser and HIR seams to extract first.

Primary targets:

- `src/frontend/parser/parser_core.cpp`
- `src/frontend/hir/hir_expr.cpp`

Actions:

- inspect current parser lookup functions for repeated namespace ancestry walk,
  alias handling, and canonical-name synthesis
- inspect `lower_expr(...)` and related helpers for heavy variable/member/call
  lowering branches and repeated argument materialization patterns
- choose a minimal extraction order that reduces inline policy without changing
  semantics

Completion check:

- a short list of parser helper seams and HIR helper seams is evident from the
  implementation structure before edits begin

## Step 2: Extract Parser Lookup Helpers

Goal: move namespace traversal and visible-name lookup mechanics behind named
parser-local helpers.

Primary targets:

- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_core.cpp`

Actions:

- centralize namespace-stack traversal used by visible lookup operations
- centralize canonical-name and `parent_id + "::" + name` key construction
- separate value/type/concept lookup policy from ancestry-walk mechanics
- reduce duplicated alias handling between visible-value and visible-type
  resolution
- keep parser entrypoints readable as grammar coordination instead of symbol
  table utility logic

Completion check:

- parser lookup behavior is routed through named helper operations with no
  intentional semantic or rollback changes

## Step 3: Extract HIR Expression-Lowering Helpers

Goal: reduce `hir_expr.cpp` to dispatcher-style orchestration plus named helper
subflows.

Primary targets:

- `src/frontend/hir/hir_expr.cpp`
- nearby HIR helper files if extraction naturally belongs there

Actions:

- keep `lower_expr(...)` as the entrypoint and dispatcher
- extract heavy variable/name, member-access, and call-lowering flows into
  dedicated helpers
- split call lowering into constructor, method, template, operator, and
  fallback helpers where those seams already exist semantically
- centralize repeated ref/rvalue-ref argument lowering and temporary
  materialization logic used across call sites

Completion check:

- major HIR expression categories are implemented through named helper seams
  rather than large inline branches

## Step 4: Validation

Goal: confirm extracted helpers preserve frontend behavior.

Actions:

- run existing parser regression coverage
- run existing HIR/frontend regression coverage
- exercise namespace-qualified lookup and `using`-driven visibility cases
- exercise dependent-name and concept-name parser coverage that relies on the
  current resolution behavior
- exercise representative HIR member/call/template lowering cases through the
  extracted paths

Completion check:

- touched parser and HIR coverage passes without new regressions attributable
  to the helper extraction
