# LIR-to-BIR Global/TypeDecl Compatibility Fence Runbook

Status: Active
Source Idea: ideas/open/185_lir_to_bir_global_typedecl_compatibility_fence.md

## Purpose

Fence the remaining LIR-to-BIR global, type declaration, and structured layout
compatibility tables so generated metadata-rich lowering cannot treat producer
final spellings as semantic authority.

## Goal

Ensure selected metadata-rich generated LIR-to-BIR global/type/layout paths use
`LinkNameId`, `LirTypeRef`, `StructNameId`, and structured layout facts for
semantic lookup, while string-keyed tables remain explicit compatibility or
display surfaces.

## Core Rule

Generated metadata-rich lowering must not silently fall back from missing or
stale structured identity metadata to final spelling as semantic authority.
Preserve raw/no-id compatibility as a fenced path.

## Read First

- `ideas/open/185_lir_to_bir_global_typedecl_compatibility_fence.md`
- `ideas/closed/180_aarch64_direct_lir_aggregate_type_bridge_retirement.md`
- `ideas/closed/183_lir_bir_backend_freeze_authority_audit.md`
- `src/codegen/lir`
- `src/backend/bir/lir_to_bir`

## Current Scope

- Audit `GlobalTypes`, `TypeDeclMap`, and `BackendStructuredLayoutTable`
  construction and consumers.
- Classify string-keyed lookup as raw/no-id compatibility, display, or
  generated-path semantic authority.
- Select one or more generated metadata-rich global/type/layout paths and
  require structured identity metadata there.
- Preserve printer/final spelling and raw imported LIR compatibility.
- Add focused tests for structured success, stale or missing metadata
  rejection, and retained raw compatibility where appropriate.

## Non-Goals

- Do not remove all legacy type declaration parsing in one pass.
- Do not rewrite aggregate layout algorithms unrelated to the selected
  boundary.
- Do not change emitted object symbol spelling.
- Do not treat final display names as unavailable; they may still be rendered
  from ids for output.
- Do not claim backend freeze closure; that remains owned by idea 188.

## Working Model

- `GlobalTypes`, `TypeDeclMap`, and structured layout tables may be valid
  compatibility bridges for raw imported LIR or no-id paths.
- Generated LIR paths should carry enough structured identity to avoid using
  producer final spelling as the semantic key.
- `LinkNameId` should own global identity where available.
- `LirTypeRef`, `StructNameId`, and structured layout entries should own type
  and aggregate layout identity where available.
- Missing or stale metadata on a selected generated path should fail closed
  instead of falling back to a string-keyed semantic table.

## Execution Rules

- Keep implementation changes scoped to the selected global/type/layout
  boundary and its direct tests.
- Treat map renames without authority changes as insufficient.
- Separate generated metadata-rich lookup from raw/no-id compatibility in code
  and tests.
- Prefer tests that exercise semantic lowering or validation behavior, not only
  printed names.
- For code-changing steps, run build proof plus focused LIR-to-BIR
  global/type/layout coverage selected by the supervisor.

## Ordered Steps

### Step 1: Inventory global/type/layout lookup surfaces

Goal: Identify the exact construction and consumer paths for the compatibility
tables named by the source idea.

Primary targets:
- `src/codegen/lir`
- `src/backend/bir/lir_to_bir`

Actions:
- Inspect `GlobalTypes`, `TypeDeclMap`, and `BackendStructuredLayoutTable`
  construction.
- Locate all consumers that use string keys or producer final spelling for
  global, type declaration, or aggregate layout decisions.
- Locate structured facts already available at those use sites, including
  `LinkNameId`, `LirTypeRef`, `StructNameId`, and structured layout entries.
- Record generated metadata-rich routes versus raw/no-id compatibility routes
  in `todo.md`.

Completion check:
- `todo.md` names the relevant table builders, consumers, existing structured
  fact sources, and fallback lookup points.

### Step 2: Select and fence a generated metadata-rich path

Goal: Require structured metadata on one or more selected generated
global/type/layout paths while preserving raw compatibility.

Primary targets:
- LIR-to-BIR global/type/layout lookup in `src/backend/bir/lir_to_bir`.
- Structured identity facts from generated LIR.

Actions:
- Add or reuse a route distinction that tells generated metadata-rich lookup
  apart from raw/no-id compatibility lookup.
- Make the selected generated path prefer structured identity and layout facts
  over final spelling.
- Make stale or missing structured metadata fail closed on the selected path.
- Keep string-keyed tables reachable only through explicit compatibility,
  display, or raw/no-id paths.

Completion check:
- The selected generated path no longer uses final spelling as normal semantic
  authority.

### Step 3: Add focused global/type/layout tests

Goal: Prove structured success, fail-closed rejection, and retained
compatibility behavior.

Primary targets:
- Existing LIR-to-BIR, backend validation, and focused backend route tests.

Actions:
- Add at least one structured success case for the selected global/type/layout
  path.
- Add at least one stale or missing structured-metadata rejection case.
- Preserve or add a raw/no-id compatibility case where string-keyed lookup
  remains intentionally allowed.
- Avoid tests that only prove pretty-printed names without exercising semantic
  lookup or validation behavior.

Completion check:
- Focused tests fail without the structured boundary and pass with it.

### Step 4: Validate and prepare handoff

Goal: Produce acceptance-grade proof for idea 185 without broadening into the
later freeze gate.

Actions:
- Run build proof and the supervisor-selected focused LIR-to-BIR
  global/type/layout coverage.
- Escalate to a broader backend or compiler subset if the implementation
  crosses shared lowering or ABI behavior beyond the selected path.
- Update `todo.md` with proof results, residual risks, and the recommended
  lifecycle close/switch decision.

Completion check:
- Idea 185 acceptance criteria are satisfied, and the supervisor has enough
  proof to review and commit the slice.
