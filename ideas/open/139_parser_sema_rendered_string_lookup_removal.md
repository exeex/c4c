# Parser Sema Rendered String Lookup Removal

Status: Open
Created: 2026-04-30
Rewritten: 2026-04-30

Historical Inputs:
- Idea 123 parser lookup removal convergence
- Idea 135 Sema structured owner/static member cleanup
- Idea 137 parser known-function spelling cleanup

Downstream Series:
- Idea 140 HIR metadata cleanup
- Idea 141 LIR metadata cleanup
- Idea 142 BIR/backend metadata cleanup

## Goal

Remove parser and Sema semantic lookup routes that rediscover identity from
rendered strings after structured metadata is available.

The desired end state is strict: parser/Sema semantic lookup uses direct AST
links, declaration/owner objects, namespace context ids, `TextId`,
`QualifiedNameKey`, `TypeSpec::record_def`, or Sema structured owner keys. A
raw rendered string may remain only as source spelling, diagnostics, display,
dump output, or final emitted text.

Renaming a helper from one label to another is not progress. A string-keyed
semantic route is progress only when it is deleted, replaced by a structured
carrier, or split into a new metadata-producing idea because the needed carrier
does not exist yet.

## Why This Idea Exists

The previous frontend cleanup rounds made many lookups structured-primary, but
some code still lets a rendered spelling decide semantic identity after a key,
owner, record, or AST carrier should already exist. That creates false
confidence: the code can look clean while preserving the same authority through
a newly named helper.

This idea exists to finish the parser/Sema removal pass before downstream HIR,
LIR, BIR, and backend cleanup depend on frontend metadata being reliable.

## Scope

- Re-inventory `src/frontend/parser` and `src/frontend/sema` for rendered-string
  semantic lookup authority.
- Remove parser typedef, value, tag, template, NTTP-default, and known-function
  rendered-string lookup routes when a structured carrier is available.
- Remove Sema owner/member/static/consteval rendered-string lookup routes when
  an owner key, direct semantic object, AST node, or declaration carrier is
  available.
- Repair parser-to-Sema metadata handoff where identity is dropped and later
  rediscovered from spelling.
- Add focused tests where a drifted rendered spelling must not affect semantic
  lookup.
- Create new open ideas for missing cross-module metadata carriers instead of
  preserving string rediscovery in parser/Sema.

## Out Of Scope

- HIR, LIR, BIR, or backend implementation cleanup, except for opening a
  metadata blocker idea.
- Removing diagnostic text, source spelling, dumps, final emitted text, or
  ABI/link-visible names.
- Weakening supported behavior, marking tests unsupported, or adding
  testcase-shaped shortcuts.
- Replacing rendered-string authority with a differently named rendered-string
  helper.

## Removal Policy

- If a structured carrier exists at the call site, a rendered string must not be
  consulted to decide semantic identity.
- If a structured carrier is missing, repair the producer or open a separate
  metadata idea. Do not keep a string rediscovery route as the plan outcome.
- `TextId` is text identity only. Prefer the domain carrier when lookup needs
  namespace, owner, record, template, value, or declaration identity.
- Tests that expect rendered spelling to win over structured metadata should be
  updated to the structured contract.
- Any retained string use must be visibly non-semantic: diagnostics, display,
  dump, source spelling, final output, or ABI/link spelling.

## Acceptance Criteria

- Covered parser/Sema semantic lookups no longer use rendered strings as an
  alternate authority once structured metadata exists.
- Known rendered-string semantic routes in parser/Sema are either removed,
  converted to structured metadata, or represented by new open metadata ideas.
- Parser-to-Sema handoff preserves the metadata needed by covered lookup paths.
- Focused frontend tests cover same-feature drifted-string cases, not just one
  named testcase.
- The final diff contains no helper-only rename packet presented as lookup
  removal.
