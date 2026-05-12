# Type Identity Authority Audit Runbook

Status: Active
Source Idea: ideas/open/172_type_identity_authority_audit.md

## Purpose

Audit type identity authority across sema, HIR, LIR, BIR, and backend lowering
before starting implementation work.

Goal: classify where type identity is structured, where it is syntax or
display, and where rendered spelling still acts as semantic authority.

Core Rule: this is an audit plan, not a type-system rewrite. Preserve current
behavior while mapping authority boundaries and producing follow-up ideas for
implementation slices.

## Read First

- `ideas/open/172_type_identity_authority_audit.md`
- `src/frontend/sema/canonical_symbol.*`
- `src/frontend/sema/type_utils.*`
- `src/frontend/sema/consteval.*`
- HIR type lowering and record/layout helpers under `src/frontend/hir/`
- LIR type references under `src/codegen/lir/`
- BIR type/layout and ABI lowering under `src/backend/bir/`

## Current Scope

- Inventory type-identity carriers and comparators.
- Classify type string usage by authority role.
- Separate syntax payload, resolved type identity, layout identity, ABI class,
  and display spelling.
- Identify high-risk gaps for follow-up implementation ideas.
- Use existing tests and route outputs to identify risk without weakening
  expectations.

## Non-Goals

- Do not rework the full type system during this audit.
- Do not reopen name/string identity cleanup unless it blocks type identity
  classification.
- Do not replace ABI printer strings or diagnostic names just because they are
  strings.
- Do not weaken backend or layout tests to match current behavior.
- Do not collapse all `TypeSpec` usage into one bucket.

## Working Model

- Treat `TypeSpec` as a mixed syntax, semantic, template, and record metadata
  carrier until each use site is classified.
- Treat rendered strings as acceptable for display, diagnostics, ABI spelling,
  compatibility bridges, or route-local rendering when they are not semantic
  authority.
- Treat equality, hashing, lookup, dedup, owner keys, and layout decisions as
  type-identity authority that must be classified precisely.
- Prefer follow-up ideas for implementation work instead of expanding this
  audit into a broad rewrite.

## Execution Rules

- Keep findings in audit artifacts or source ideas requested by the supervisor;
  do not edit implementation code under this plan-owner activation.
- For later executor packets, make code-reading proof explicit: name the files
  inspected, the authority classification found, and any test or route output
  used to support the risk assessment.
- When a gap needs implementation, record it as a concrete follow-up idea with
  reviewer reject signals instead of silently broadening this active runbook.
- Flag any temptation to downgrade tests or expectation contracts as route
  drift.

## Steps

### Step 1: Inventory Sema Type Authority

Goal: map sema-owned type identity surfaces and separate canonical identity
from syntax or display payload.

Primary targets:
- `TypeSpec` definitions and helpers
- `src/frontend/sema/canonical_symbol.*`
- `src/frontend/sema/type_utils.*`
- `src/frontend/sema/consteval.*`

Actions:
- Inspect `TypeSpec` fields and comparison/canonicalization helpers.
- Find equality, hashing, lookup, binding, and dedup sites that use type names,
  partial `TypeSpec` comparison, or rendered strings.
- Classify sema uses as syntax payload, resolved type identity, display,
  diagnostics, or compatibility bridge.
- Note unresolved ambiguity for records, typedefs, templates, arrays, and
  function pointer types.

Completion check:
- Sema audit notes distinguish canonical identity from syntax/display payload
  and identify the highest-risk authority gaps.

### Step 2: Inventory HIR Type And Record Authority

Goal: map HIR type refs, template-substituted types, struct owner/layout keys,
and pending-work keys.

Primary targets:
- `src/frontend/hir/`
- HIR type lowering helpers
- HIR record/layout ownership helpers

Actions:
- Inspect HIR type refs and lowering paths from AST/sema into HIR.
- Identify where substituted `TypeSpec`s carry identity versus syntax context.
- Find record owner keys, layout keys, pending-work maps, and dedup points.
- Classify rendered type or struct names as semantic authority, display,
  compatibility bridge, or route-local rendering.

Completion check:
- HIR audit notes separate type identity, record/layout identity, and display
  spelling, with concrete risk entries for ambiguous authority.

### Step 3: Inventory LIR/BIR Type And Layout Authority

Goal: map backend-facing type identity, layout mirrors, ABI classes, and
string-rendered type references.

Primary targets:
- `src/codegen/lir/`
- `src/backend/bir/`
- backend ABI/layout helpers

Actions:
- Inspect LIR type references and their conversion into BIR or backend routes.
- Identify BIR type kinds, structured layout mirrors, ABI aggregate helpers,
  and any rendered-name comparisons.
- Classify type strings as semantic authority, ABI spelling, display/output,
  compatibility bridge, or route-local rendering.
- Note layout and ABI decisions that depend on spelling instead of structured
  keys.

Completion check:
- LIR/BIR/backend audit notes distinguish structured type/layout authority
  from output spelling and identify priority gaps.

### Step 4: Cross-Domain Risk Map

Goal: connect sema, HIR, LIR, BIR, and backend findings into one authority map.

Actions:
- Group findings by domain: syntax payload, resolved type identity, layout
  identity, ABI class, display spelling, diagnostics, and compatibility bridge.
- Prioritize gaps by collision risk, stale compatibility behavior, and backend
  observability.
- Identify nearby existing tests or route outputs that demonstrate each risk
  without rewriting expectations.
- Mark any areas where name-identity migration gaps block type-identity
  classification.

Completion check:
- A coherent type-identity audit exists and remaining spelling-based authority
  is classified and prioritized.

### Step 5: Write Follow-Up Implementation Ideas

Goal: turn the highest-risk gaps into bounded implementation initiatives.

Actions:
- Create concrete follow-up idea files under `ideas/open/` only for gaps that
  require implementation beyond this audit.
- For each follow-up idea, include scope, non-goals, acceptance criteria, and
  reviewer reject signals tied to type identity.
- Keep each follow-up small enough to validate with build proof plus focused
  route or CTest coverage.
- Do not use follow-up idea creation to claim implementation progress.

Completion check:
- The highest-risk type identity gaps have concrete follow-up ideas and this
  audit remains separated from implementation.

### Step 6: Audit Closure Check

Goal: decide whether the source idea is complete or whether the runbook needs
replacement.

Actions:
- Verify the audit distinguishes syntax payload, resolved type identity,
  layout identity, ABI class, and display spelling.
- Verify all remaining spelling-based type authority is classified.
- Verify existing tests were used only as evidence and were not weakened.
- Ask the supervisor to route closure through plan-owner with regression guard
  only after the source idea's acceptance criteria are satisfied.

Completion check:
- The source idea can be closed only when its audit and follow-up criteria are
  satisfied; otherwise keep or replace the runbook without closing the idea.
