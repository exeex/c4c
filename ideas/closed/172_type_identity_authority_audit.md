# Type Identity Authority Audit

Status: Closed
Created: 2026-05-11
Closed: 2026-05-12

Depends On:
- `ideas/open/171_identity_authority_migration_closure_gate.md`

## Goal

Start the next migration theme: audit type identity authority across sema,
HIR, LIR, BIR, and backend lowering.

The previous migration made name identity less dependent on rendered strings.
The next risk is type identity depending on spelling, partial `TypeSpec`
comparison, rendered struct names, ad hoc suffix strings, or duplicated
canonicalization logic instead of structured type/domain identity.

## Why This Idea Exists

Once name identity is stable, type identity becomes the next likely source of
subtle collisions and stale compatibility behavior:

- `TypeSpec` carries syntax, semantic, template, and record metadata in one
  broad struct.
- sema has `CanonicalType`, consteval type bindings, and type utility helpers.
- HIR has type refs, template-substituted `TypeSpec`s, struct owner/layout
  identity, and type-based pending work.
- LIR/BIR preserve both structured type refs and rendered type strings.
- backend route tests often observe type/layout behavior through final
  spelling or BIR text.

This idea should map those domains before implementation work begins.

## In Scope

- Inventory type-identity carriers and comparators across:
  `TypeSpec`, sema canonical types, HIR type refs, struct/record owner keys,
  template-substituted types, LIR type refs, BIR type kinds, structured layout
  mirrors, and backend ABI/layout helpers.
- Classify type string usage as semantic authority, display/output,
  diagnostic, ABI spelling, compatibility bridge, or route-local rendering.
- Identify where type equality, hashing, lookup, or dedup relies on rendered
  names or ad hoc string suffixes.
- Identify where `TypeSpec` is acting as syntax payload versus resolved type
  identity.
- Identify missing structured type ids or domain keys, especially for records,
  typedefs, template instantiations, function pointer types, arrays, and ABI
  aggregate layouts.
- Produce follow-up ideas for implementation slices instead of doing a giant
  type-system rewrite in the audit.

## Out Of Scope

- Reworking the full type system during the audit.
- Reopening name/string identity cleanup unless it blocks type identity.
- Replacing ABI printer strings or diagnostic type names just because they are
  strings.
- Weakening backend/type-layout tests to match current behavior.

## Candidate Anchors

- `TypeSpec` and template argument/type metadata in parser AST structures.
- `src/frontend/sema/canonical_symbol.*`
- `src/frontend/sema/type_utils.*`
- `src/frontend/sema/consteval.*`
- HIR type lowering and record/layout helpers under `src/frontend/hir/`.
- LIR type references under `src/codegen/lir/`.
- BIR type/layout and ABI lowering under `src/backend/bir/`.
- backend prealloc/ABI helpers that compare or render aggregate types.

## Acceptance Criteria

- A type-identity audit exists and distinguishes syntax payload, resolved type
  identity, layout identity, ABI class, and display spelling.
- Remaining spelling-based type authority is classified and prioritized.
- Concrete follow-up ideas exist for the highest-risk type identity gaps.
- The audit does not collapse all `TypeSpec` usage into one bucket.
- Existing tests are used to identify risk; they are not weakened.

## Reviewer Reject Signals

- The audit treats all type strings as semantic bugs.
- `TypeSpec` syntax payload and resolved type identity are conflated.
- A rendered type spelling remains semantic authority without classification.
- The work expands into a large type-system rewrite before follow-up ideas are
  written.

## Closure Notes

- The audit completed as read-only lifecycle work across sema, HIR, LIR, BIR,
  and backend type/layout/ABI authority surfaces.
- The audit distinguished syntax payload, resolved type identity, layout
  identity, ABI class, display spelling, diagnostics, and compatibility
  bridges in the Step 1-4 `todo.md` history.
- Remaining spelling-based type authority was classified and prioritized in
  the Step 4 cross-domain risk map.
- Follow-up implementation work was separated into open ideas 173-177 for the
  highest-risk gaps: aggregate layout identity, aggregate ABI classification,
  HIR `TypeSpec` ref equivalence, `LirTypeRef` structured equality, and
  template record owner identity.
- No implementation code or tests were weakened as part of this audit.
