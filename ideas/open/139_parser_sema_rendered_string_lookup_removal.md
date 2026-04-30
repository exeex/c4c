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

The desired end state is strict: parser/Sema semantic lookup APIs must not take
`std::string`, `std::string_view`, rendered spelling, or compatibility fallback
arguments. Semantic lookup uses direct AST links, declaration/owner objects,
namespace context ids, `TextId`, `QualifiedNameKey`, `TypeSpec::record_def`, or
Sema structured owner keys. A raw string may remain only as source spelling,
diagnostics, display, dump output, ABI/link spelling, or final emitted text.

Renaming a helper from one label to another is not progress. A string or
`std::string_view` semantic lookup route is progress only when it is deleted,
replaced by a structured carrier, or split into a new metadata-producing idea
because the needed carrier does not exist yet.

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
- Remove semantic lookup interfaces that accept `std::string`,
  `std::string_view`, rendered spelling, or `TextId` plus a fallback spelling.
- Collapse overload families that keep both a string route and a structured
  route into a single `TextId` or domain-key API.
- Remove parser/Sema semantic API names containing `fallback` or `legacy` when
  those names preserve string-based lookup compatibility.
- Use `map<TextId>` and `set<TextId>` style lookup tables when text identity is
  the intended semantic key; `TextId` itself does not carry namespace, owner, or
  declaration meaning.
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
- Semantic lookup APIs must not accept `std::string`, `std::string_view`, or a
  fallback spelling parameter. For example,
  `has_typedef_name(TextId name_text_id, std::string_view fallback_name)` must
  become `has_typedef_name(TextId name_text_id)`.
- Do not keep overload families where one overload uses `std::string` or
  `std::string_view` and another uses `TextId`, `QualifiedNameKey`, or a domain
  key. Collapse the family to the structured/domain-key route.
- Remove semantic API names containing `fallback` or `legacy` when they refer to
  compatibility lookup through rendered spelling.
- `TextId` is text identity only. Prefer the domain carrier when lookup needs
  namespace, owner, record, template, value, or declaration identity. Use
  `map<TextId>` or `set<TextId>` when text identity itself is the lookup domain.
- Tests that expect rendered spelling to win over structured metadata should be
  updated to the structured contract.
- Any retained string use must be visibly non-semantic: diagnostics, display,
  dump, source spelling, final output, or ABI/link spelling.

## Acceptance Criteria

- Covered parser/Sema semantic lookups no longer use rendered strings as an
  alternate authority once structured metadata exists.
- Parser/Sema semantic lookup interfaces no longer accept `std::string`,
  `std::string_view`, rendered spelling, or `TextId` plus fallback spelling
  arguments.
- String and structured overload families have been collapsed to `TextId` or
  domain-key APIs, with `TextId`-keyed maps/sets used only where text identity
  is the intended lookup key.
- Semantic parser/Sema APIs named `fallback` or `legacy` have been removed
  unless the retained use is visibly non-semantic.
- Known rendered-string semantic routes in parser/Sema are either removed,
  converted to structured metadata, or represented by new open metadata ideas.
- Parser-to-Sema handoff preserves the metadata needed by covered lookup paths.
- Focused frontend tests cover same-feature drifted-string cases, not just one
  named testcase.
- The final diff contains no helper-only rename packet presented as lookup
  removal.

## Reviewer Reject Signals

- A semantic lookup API still accepts `std::string`, `std::string_view`,
  rendered spelling, or `TextId` plus fallback spelling after the route is
  claimed complete.
- A string overload is retained beside a `TextId`, `QualifiedNameKey`, owner, or
  domain-key overload for the same parser/Sema semantic lookup family.
- A parser/Sema semantic helper keeps `fallback` or `legacy` in its name to
  preserve rendered-spelling compatibility.
- The change replaces one rendered-string lookup helper with another wrapper
  instead of deleting the string route or adding a real metadata carrier.
- Tests are weakened, marked unsupported, or rewritten around one named case
  instead of proving same-feature structured-vs-rendered disagreement.
- A `TextId` route is treated as carrying namespace, owner, declaration, record,
  template, or value semantics that require a richer domain key.
