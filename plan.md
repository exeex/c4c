# Sema HIR AST Ingress Boundary Audit Runbook

Status: Active
Source Idea: ideas/open/130_sema_hir_ast_ingress_boundary_audit.md

## Purpose

Make the Sema and HIR AST ingress boundary explicit so later cleanup can tell
which fields carry semantic authority and which strings are only display or
compatibility text.

## Goal

Mark, review, and classify Sema/HIR ingress points that consume parser AST
fields, without replacing lookup logic or redesigning ownership.

## Core Rule

Do not treat rendered spelling or `std::string` text as cross-stage semantic
authority unless no structured carrier exists; classify legitimate generated or
display text separately from lookup identity.

## Read First

- `ideas/open/130_sema_hir_ast_ingress_boundary_audit.md`
- `src/frontend/sema/validate.hpp`
- `src/frontend/sema/validate.cpp`
- `src/frontend/sema/canonical_symbol.hpp`
- `src/frontend/sema/canonical_symbol.cpp`
- `src/frontend/hir/hir.hpp`
- `src/frontend/hir/hir_lowering_core.cpp`
- `src/frontend/hir/hir_build.cpp`

## Current Targets

- Sema AST ingress helpers around `SemaStructuredNameKey`
- `ResolvedTypeTable` production and consumption boundaries
- HIR `NamespaceQualifier` construction from AST fields
- HIR `TextId` or name construction from AST spelling
- Related HIR `impl/` lowering helpers only as needed to classify ingress

## Non-Goals

- Do not replace HIR lookup logic.
- Do not redesign `ResolvedTypeTable`.
- Do not change AST lifetime or ownership.
- Do not remove compatibility string fields from HIR as part of this audit.
- Do not expand this into a bulk string-removal project.

## Working Model

- Parser AST `TextId` and structured fields can be semantic input authority.
- Sema outputs, including resolved type tables keyed by `const Node*`, can be
  semantic authority for downstream stages.
- HIR module tables can become authority after lowering.
- `std::string`, rendered names, diagnostics, dumps, mangling, anonymous names,
  section labels, and final display text are valid only when they are not the
  cross-stage lookup authority.

## Execution Rules

- Keep the audit focused on Sema/HIR ingress surfaces named by the source idea.
- Prefer comments, local notes, or narrow follow-up idea creation over broad
  code changes when classification is enough.
- If a suspicious string-authority path needs implementation work, record it as
  a separate `ideas/open/` follow-up instead of silently expanding this plan.
- Preserve behavior for existing frontend and HIR tests.
- For any code or comment changes, run at least a build or compile proof plus
  the narrow frontend/HIR tests selected by the supervisor.

## Ordered Steps

### Step 1: Inspect Sema AST Ingress

Goal: Classify how Sema consumes parser AST fields and structured names.

Primary target: `src/frontend/sema/validate.*` and
`src/frontend/sema/canonical_symbol.*`.

Actions:

- Inspect `SemaStructuredNameKey` creation and use.
- Inspect `ResolvedTypeTable` production and consumption at AST boundaries.
- Search nearby Sema helpers for `std::string`, raw spelling, rendered names,
  and fallback lookup keys.
- Classify each relevant path as structured authority, Sema authority,
  compatibility fallback, display text, or suspicious string authority.

Completion check:

- Sema ingress paths have clear classification notes in the appropriate layer,
  and any suspicious Sema-only cleanup has a concrete follow-up target.

### Step 2: Inspect HIR AST Ingress

Goal: Classify how HIR lowering consumes AST fields and Sema outputs.

Primary target: `src/frontend/hir/hir.hpp`,
`src/frontend/hir/hir_lowering_core.cpp`, `src/frontend/hir/hir_build.cpp`,
and related `src/frontend/hir/impl/` helpers as needed.

Actions:

- Inspect HIR `NamespaceQualifier` construction from AST fields.
- Inspect HIR name and `TextId` creation from AST spelling.
- Trace where `ResolvedTypeTable` or other Sema outputs override raw AST text.
- Search the ingress helpers for `std::string`, rendered names, raw spelling,
  and string-keyed maps used as semantic lookup authority.
- Classify legitimate generated names, mangling, anonymous labels, diagnostics,
  dump text, and final display text separately from cross-IR identity.

Completion check:

- HIR ingress paths have clear classification notes in the appropriate layer,
  and any suspicious HIR cleanup has a concrete follow-up target.

### Step 3: Record Follow-Up Work For Suspicious Authority

Goal: Keep this audit from turning into broad implementation work while still
preserving durable cleanup targets.

Actions:

- For each string-authority path that cannot be safely resolved inside this
  audit, create or update a focused follow-up idea under `ideas/open/`.
- Keep follow-up ideas narrow enough to avoid testcase-shaped cleanup.
- Do not edit unrelated open ideas unless the suspicious path directly belongs
  to that idea's durable scope.

Completion check:

- Every suspicious cross-stage string authority found by Steps 1 and 2 is
  either classified as acceptable or mapped to a focused follow-up idea.

### Step 4: Validate Behavior And Close Audit Evidence

Goal: Prove the audit did not change frontend/HIR behavior and leave enough
evidence for the supervisor to decide closure or next activation.

Actions:

- Run the supervisor-delegated build or compile proof.
- Run the supervisor-delegated frontend/HIR test subset.
- Update `todo.md` with the latest proof command and result.
- Summarize the final classification outcome without promoting routine packet
  detail into the source idea.

Completion check:

- Fresh proof is recorded in `todo.md`.
- Existing frontend/HIR behavior remains unchanged.
- The supervisor can tell whether the source idea is complete or whether a new
  runbook/follow-up activation is needed.
