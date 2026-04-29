# Parser AST Handoff Role Labeling Runbook

Status: Active
Source Idea: ideas/open/128_parser_ast_handoff_role_labeling.md

## Purpose

Make the Parser AST carrier fields self-documenting before later lookup and
carrier cleanup work changes behavior.

## Goal

Label `TypeSpec`, `TemplateArgRef`, and `Node` fields by boundary role while
preserving the flat AST carrier model and existing behavior.

## Core Rule

This is a documentation and organization pass only. Do not change parser, Sema,
HIR, codegen, or test behavior.

## Read First

- `ideas/open/128_parser_ast_handoff_role_labeling.md`
- `src/frontend/parser/ast.hpp`
- Existing nearby comments in `TypeSpec`, `TemplateArgRef`, and `Node`

## Current Targets

- `src/frontend/parser/ast.hpp`
- Follow-up idea creation under `ideas/open/` only if suspicious cross-stage
  string or rendered-spelling authority is found during review.

## Non-Goals

- Do not move fields out of `Node`, `TypeSpec`, or `TemplateArgRef`.
- Do not introduce access-control labels, encapsulation, or new carrier types.
- Do not replace string lookup paths with `TextId` paths.
- Do not modify parser, Sema, HIR, backend, or test expectations for behavior.

## Working Model

- Cross-stage contract fields are the AST payload expected to be consumed after
  parsing by Sema, HIR, or later lowering.
- Parser-produced semantic hints are parser-derived facts that help downstream
  stages but are not the primary identity authority.
- Compatibility/display spelling is retained text for diagnostics, dumps,
  legacy bridges, or final display.
- Recovery/debug placeholders support parser recovery, diagnostics, or
  debugging and should not be treated as semantic authority.
- Legacy bridge fields may remain but should be explicitly marked as not the
  preferred authority.

## Execution Rules

- Prefer comments and grouping over structural changes.
- Reorder fields only when the move is behavior-neutral and keeps related
  handoff payload together.
- Keep comments concrete enough that a future cleanup can tell which fields
  cross the parser boundary.
- If review finds suspicious cross-stage `std::string` or rendered spelling
  authority, record it as a follow-up idea instead of fixing it in this runbook.
- For code-changing documentation edits, prove the affected frontend still
  builds or compiles with behavior unchanged.

## Ordered Steps

### Step 1: Inspect Current AST Carrier Layout

Goal: Build an exact map of the existing `TypeSpec`, `TemplateArgRef`, and
`Node` field groups before editing comments.

Primary target: `src/frontend/parser/ast.hpp`

Actions:
- Inspect the current field order and nearby comments in `TypeSpec`,
  `TemplateArgRef`, and `Node`.
- Identify fields that are already grouped by shape but should instead be
  labeled by boundary role.
- Note any cross-stage `std::string` or rendered-spelling fields that require
  explicit compatibility or legacy-bridge labeling.

Completion check:
- The executor can name the field groups that will receive role labels before
  making edits.

### Step 2: Label `TypeSpec` And `TemplateArgRef`

Goal: Make parser type-reference carriers explicit about durable identity,
semantic hints, compatibility spelling, and debug/recovery payload.

Primary target: `src/frontend/parser/ast.hpp`

Actions:
- Add or tighten section comments for `TypeSpec` fields by role.
- Add or tighten section comments for `TemplateArgRef` fields by role.
- Keep the flat data model intact and avoid behavior-affecting changes.
- Reorder fields only if the move is layout-neutral for behavior and makes the
  role grouping clearer.

Completion check:
- `TypeSpec` and `TemplateArgRef` comments distinguish cross-stage contract
  payload from parser-local or compatibility/display spelling.

### Step 3: Label `Node`

Goal: Make `Node` payload roles explicit for downstream parser AST consumers.

Primary target: `src/frontend/parser/ast.hpp`

Actions:
- Add or tighten section comments for `Node` fields by role.
- Mark compatibility/display spelling and legacy bridge fields as such.
- Mark parser recovery/debug payload separately from semantic handoff fields.
- Keep the carrier flat and behavior-preserving.

Completion check:
- A reader can tell which `Node` fields are intended for cross-stage
  consumption after parsing and which fields are compatibility, display,
  recovery, or legacy bridge payload.

### Step 4: Capture Suspicious String Authority Follow-Ups

Goal: Preserve any discovered cleanup work without expanding this documentation
pass into behavior changes.

Primary target: `ideas/open/`

Actions:
- Review the edited labels for any `std::string` or rendered spelling that
  appears to be cross-stage semantic authority.
- If such authority exists and is not already covered by an open idea, create a
  narrow follow-up idea under `ideas/open/`.
- Do not fix the behavior in this runbook.

Completion check:
- Suspicious string or rendered-spelling authority found during AST carrier
  review is either explicitly labeled as compatibility/legacy payload or has a
  concrete follow-up idea.

### Step 5: Prove No Behavior Change

Goal: Show the labeling pass did not change frontend behavior.

Primary target: build or frontend/parser proof selected by the supervisor.

Actions:
- Run the narrow compile or test command delegated by the supervisor.
- Escalate only if the comment/layout changes unexpectedly affect compilation
  or behavior.

Completion check:
- Fresh proof is recorded in `todo.md` by the executor, with no behavior
  regressions attributed to this runbook.
