# HIR TypeSpec Ref Structured Equivalence

Status: Closed
Created: 2026-05-12
Closed: 2026-05-12

Depends On:
- `ideas/open/172_type_identity_authority_audit.md`

## Goal

Separate ordinary HIR type-reference identity from raw or partial `TypeSpec`
equivalence so function pointers, arrays, typedefs, records, and template
records can be compared through structured resolved identity where available.

## Why This Idea Exists

The audit found that `TypeSpec` remains a broad syntax and metadata carrier
through parser, sema, HIR, and older LIR surfaces. Sema owns stronger resolved
identity through canonical type and symbol structures, and HIR already
preserves some structured identity for callable function-pointer signatures and
owner-aware template bindings. Ordinary `QualType`, field, local, global, and
parameter paths still commonly carry `TypeSpec` first, which creates collision
risk when raw or partial `TypeSpec` comparison decides equality, binding,
lookup, or dedup.

## In Scope

- Pick one HIR ordinary type-ref family where raw `TypeSpec` equivalence still
  decides semantic identity.
- Introduce or thread structured resolved type identity for that family when
  sema/HIR already has the necessary facts.
- Keep `TypeSpec` as syntax/declarator payload where it is genuinely carrying
  syntax, qualifiers, unresolved array expressions, or display context.
- Make no-metadata compatibility explicit and avoid broad migration of all
  `TypeSpec` uses in one slice.
- Prove with build proof plus focused frontend/HIR tests for the chosen type
  family.

## Out Of Scope

- Collapsing all `TypeSpec` usage into a single replacement type.
- Rewriting the complete HIR type lowering pipeline.
- Treating display formatting or diagnostics as bugs merely because they use
  `TypeSpec`.
- Adding testcase-shaped special cases for one named typedef, record, array, or
  function-pointer example.

## Acceptance Criteria

- The chosen HIR type-ref path can distinguish syntax payload from semantic
  type identity in code and tests.
- Equality, lookup, binding, or dedup touched by the slice uses structured
  resolved identity where present and does not accept equal raw `TypeSpec`
  spelling as sufficient.
- Existing syntax/display uses of `TypeSpec` remain intact and are documented
  by code shape or focused comments only where needed.
- Focused tests cover at least one nearby collision-prone domain such as
  function pointers, arrays, typedefs, records, or template-origin records.
- The proof includes a fresh build or compile check plus targeted frontend/HIR
  CTest coverage.

## Completion Notes

Closed after the focused HIR aggregate direct-assignment slice selected one
ordinary HIR type-ref family and moved the metadata-rich owner comparison away
from rendered `TypeSpec` spelling.  The accepted route requires structured
aggregate owner identity for compound-literal direct assignment while retaining
`TypeSpec` as syntax/display payload.

Proof used the focused `frontend_hir_lookup_tests` command, adjacent
`frontend_hir` validation, and the accepted full-suite baseline at `b13b6d431`
with 3137/3137 passing.  Close-gate regression guard compared canonical
`test_before.log` and `test_after.log` for `frontend_hir_lookup_tests`; the
only log diff was total runtime, and non-decreasing regression comparison
passed with 1/1 tests passing before and after.

## Reviewer Reject Signals

- The route adds a narrow named-case comparison or another rendered type-name
  branch instead of moving the chosen HIR path to structured identity.
- The diff treats all `TypeSpec` fields as semantic identity and breaks syntax
  payload, declarator, unresolved array, or display behavior.
- Tests are weakened or rewritten to avoid checking the collision-prone type
  family that motivated the slice.
- Helper renames are claimed as capability progress while raw or partial
  `TypeSpec` equality still decides the same lookup, binding, or dedup point.
- The slice expands into an unbounded type-system rewrite without finishing
  one focused HIR type-ref family.
