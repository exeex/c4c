# Template Record Owner Structured Identity

Status: Open
Created: 2026-05-12

Depends On:
- `ideas/open/172_type_identity_authority_audit.md`

## Goal

Remove rendered `SpecializationKey::canonical` text as semantic authority from
template record owner identity and replace it with structured specialization
identity where the required owner facts are available.

## Why This Idea Exists

The audit found that template binding and specialization identity have become
more structured, but template record/layout owner identity still carries a
serialized `SpecializationKey::canonical` component through
`HirRecordOwnerTemplateIdentity`. That rendered component is a stale
compatibility risk for record lookup, layout ownership, and downstream
aggregate identity because the owner key can still accept equal spelling as
semantic identity.

## In Scope

- Pick one template record owner path where `SpecializationKey::canonical`
  participates in record or layout identity.
- Thread structured specialization identity, owner indexes, or domain keys
  already available from parser/sema/HIR into that owner path.
- Keep rendered specialization text for display, dumps, diagnostics, and
  compatibility-only surfaces.
- Make missing structured owner data explicit instead of silently treating the
  canonical string as authoritative for metadata-rich paths.
- Prove with build proof plus focused HIR template record or layout-owner
  tests.

## Out Of Scope

- Rewriting all template instantiation and substitution machinery.
- Removing display names, dump keys, or diagnostic text for template
  specializations.
- Solving unrelated name-identity compatibility bridges unless they directly
  block the selected template record owner path.
- Adding special-case logic for one rendered template spelling.

## Acceptance Criteria

- The selected template record owner identity path no longer depends on
  `SpecializationKey::canonical` as the primary semantic key when structured
  specialization identity is present.
- Record lookup, layout ownership, or owner dedup in that path can reject or
  report mismatched structured identity even if rendered canonical text would
  match.
- Rendered specialization strings remain output/diagnostic payloads and are
  visibly separated from semantic owner identity.
- Focused tests cover a template record owner or layout-owner case where
  structured identity matters beyond equal display spelling.
- The proof includes a fresh build or compile check plus targeted HIR/frontend
  CTest coverage.

## Reviewer Reject Signals

- The route normalizes, reparses, or compares `SpecializationKey::canonical`
  differently and claims that as structured owner identity.
- A metadata-rich template record owner still falls back silently to canonical
  rendered text when structured specialization facts are missing or mismatched.
- Tests only check dump names or mangled/rendered spelling, not owner identity,
  record lookup, or layout ownership behavior.
- The diff weakens template record/layout tests or changes expectations without
  capability improvement.
- The slice becomes a broad template-system rewrite instead of finishing one
  bounded owner-identity path.
