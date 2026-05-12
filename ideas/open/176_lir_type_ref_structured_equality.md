# LIR Type Ref Structured Equality

Status: Open
Created: 2026-05-12

Depends On:
- `ideas/open/172_type_identity_authority_audit.md`

## Goal

Make `LirTypeRef` equality respect structured payload, especially
`StructNameId`, so equal rendered text cannot mask missing or mismatched type
identity metadata.

## Why This Idea Exists

The audit found that LIR has `LirTypeRef` and optional structured ids, but
`LirTypeRef::operator==` still compares rendered text. That means new ids
cannot reliably defend dedup, mirror checks, generated LIR validation, or
backend handoff while equal spelling remains sufficient equality. This is a
medium-high risk because it can hide missing `StructNameId` data exactly where
the migration expects structured identity to be protective.

## In Scope

- Update one bounded `LirTypeRef` equality or comparison path so structured
  metadata participates in equality when present.
- Define clear compatibility behavior for legacy/no-id LIR inputs.
- Add or adjust focused tests that catch equal text with missing or mismatched
  structured ids in the selected path.
- Keep display/printer text stable unless the selected proof route already
  requires a deliberate metadata-observability change.
- Prove with build proof plus targeted frontend LIR or backend handoff tests.

## Out Of Scope

- Retiring every LIR text field in one slice.
- Changing final rendered LIR syntax merely to show new ids.
- Replacing all backend type lowering routes at the same time.
- Treating no-id hand-authored legacy fixtures the same as generated
  metadata-rich LIR without an explicit compatibility decision.

## Acceptance Criteria

- The selected `LirTypeRef` comparison no longer treats equal rendered text as
  sufficient when structured ids are present.
- Missing or mismatched `StructNameId` data is observable through a focused
  verifier, route, or unit test rather than silently accepted.
- Legacy/no-id compatibility remains intentional and does not leak into
  generated metadata-rich paths.
- Existing frontend LIR type-ref tests remain strong and are extended or paired
  with a focused collision test.
- The proof includes a fresh build or compile check plus targeted frontend LIR
  or backend CTest coverage.

## Reviewer Reject Signals

- The route only changes the rendered `LirTypeRef` string or parser branch and
  leaves `operator==` or the selected comparison text-only.
- Mismatched structured ids still compare equal when rendered spelling matches
  in a metadata-rich path.
- Tests are changed to assert only printed text while structured equality is
  untested.
- The diff hides missing metadata by synthesizing ids from rendered names at
  the comparison site.
- The slice broadens into unrelated LIR printer/backend rewrites without
  proving the structured equality contract.
