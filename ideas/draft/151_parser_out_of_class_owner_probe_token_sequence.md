# Parser Out Of Class Owner Probe Token Sequence

Status: Draft
Created: 2026-05-06

Parent Ideas:
- `ideas/closed/139_parser_sema_rendered_string_lookup_removal.md`
- `ideas/closed/145_move_record_tag_authority_from_parser_to_sema.md`
- `ideas/open/146_qualified_name_deferred_carrier_authority.md`

## Goal

Remove rendered-string authority from parser out-of-class member and
constructor owner probing.

The parser should detect lexical owners from token `TextId` sequences and
structured qualified-owner keys before deciding whether a declaration denotes
an out-of-class member, constructor, destructor, conversion, or operator
definition. A rendered owner string may remain for diagnostics and display,
but should not be the decision key.

Parser may classify special-member syntax from token shape, but owner identity
must remain structured. Sema should resolve the owner eagerly when possible.
HIR may consume resolved or deferred owner carriers later, but no layer should
decide owner identity by formatting and comparing a rendered `qualified_owner`
string.

## Why This Idea Exists

Out-of-class special-member parsing currently has owner-probing paths that can
build `qualified_owner` strings before deciding the actual owner shape.
Current anchors include `src/frontend/parser/impl/declarations.cpp`, especially
helpers around `probe_special_member_owner` and
`consume_special_member_owner`.

That route keeps a fragile rendered-string dependency in a grammar-sensitive
part of the parser. The intended policy is:

- Lexer interns spelling into `TextId`.
- `TextId` carries no semantics beyond spelling identity.
- Semantic meaning lives in domain tables.
- Compound owners are indexed by token `TextId` sequences and structured
  qualified-owner keys, not by rendered strings.

This idea exists to make owner probing preserve lexical structure directly
from source tokens, so operator and constructor classification does not depend
on formatting a qualified owner and then interpreting that text. The parser's
job is declaration-shape classification and structured owner carrier
production; semantic owner truth belongs to Sema or, for deferred/template
cases, late HIR resolution through structured carriers.

## Working Responsibility Split

### Parser

Parser owns grammar-sensitive owner probing.

Parser should:

- recognize out-of-class declaration shape from token sequence
- classify constructor, destructor, conversion, operator, and member syntax
  using structured name components
- produce a structured owner-probe carrier with owner qualifier `TextId`
  sequence, base/member `TextId`, global qualifier, source span where possible,
  and a qualified-owner key when available
- preserve rendered owner spelling only for diagnostics/debug output

Parser should not:

- decide final class/namespace/member owner identity from a rendered owner
  string
- treat a single `TextId` containing `A::B` as semantic owner identity
- use formatted `qualified_owner` text as the authoritative constructor or
  operator classification key

### Sema

Sema owns semantic owner resolution when enough information is available.

Sema should:

- verify whether the structured owner denotes a class, namespace, or other
  valid declaration owner
- resolve non-dependent owner identities through domain tables
- produce a structured deferred owner carrier for dependent/template owner
  cases

### HIR

HIR may complete late owner resolution for deferred/template-dependent cases.

HIR should:

- consume resolved owner identity or structured deferred owner carriers
- finish owner lookup under template substitution when required
- lower the result into HIR/module-domain function/member/record keys

HIR should not:

- split rendered owner strings
- rediscover owner identity from display spelling
- treat parser owner-probe compatibility strings as late semantic authority

## In Scope

- Inventory `probe_special_member_owner`, `consume_special_member_owner`, and
  nearby out-of-class declaration paths in
  `src/frontend/parser/impl/declarations.cpp`.
- Identify every place where `qualified_owner` or equivalent rendered owner
  spelling influences constructor, destructor, conversion, operator, or member
  owner decisions.
- Define a structured owner-probe result that carries token `TextId`
  sequence, qualifier segmentation, source spans where available, and a
  qualified-owner/domain key when the parser has enough context to build one.
- Ensure lexical detection for constructors, destructors, conversion
  functions, and operators compares structured name components rather than
  rendered owner strings.
- Ensure Sema/HIR consumers can receive resolved owner identity or a structured
  deferred owner carrier without falling back to rendered owner spelling.
- Preserve rendered owner spelling for diagnostics, debug dumps, and
  compatibility mirrors only.
- Add tests or focused probes where nested owners, same-spelling local names,
  operator declarations, and constructors would be misclassified by flattened
  owner text.
- Document any remaining rendered owner compatibility path with owner,
  limitation, and removal condition.

## Out Of Scope

- Full C++ member lookup or overload resolution redesign.
- Moving all declaration semantic authority from parser to Sema in this idea.
- Forcing all owner resolution to finish in Sema before HIR.
- Treating one `TextId` for a rendered owner spelling as semantic owner
  identity.
- Removing diagnostic or dump output for owner names.
- Broad HIR/backend/codegen rewrites beyond narrow consumer changes needed to
  preserve structured owner metadata.
- Weakening tests or marking out-of-class member cases unsupported.

## Acceptance Criteria

- Out-of-class owner probing produces and consumes a structured owner-probe
  result based on token `TextId` sequence and qualified-owner/domain key
  metadata.
- Constructor, destructor, conversion, operator, and member-owner decisions no
  longer depend on rendered `qualified_owner` string comparison as their
  authoritative route.
- Sema/HIR owner resolution, where required beyond parser shape
  classification, consumes structured owner identity or deferred owner
  carriers rather than rendered owner strings.
- Rendered owner spelling, if retained, is display-only or temporary
  compatibility with a documented removal condition.
- Tests or focused probes cover a case where flattened owner spelling would
  misclassify the declaration shape or owner.
- The route preserves the policy that `TextId` is spelling identity only and
  that compound owner meaning lives in structured qualified-owner keys/domain
  tables.

## Reviewer Reject Signals

- A slice claims progress by renaming `qualified_owner` while still deciding
  owner shape through rendered string comparison.
- The implementation treats one rendered-owner `TextId` as a complete
  semantic owner key.
- Operator or constructor handling gains named-case shortcuts instead of a
  general token-sequence owner probe.
- HIR late owner resolution splits or compares rendered owner strings instead
  of consuming structured owner carriers.
- Tests are weakened, marked unsupported, or narrowed to avoid nested owners,
  operators, constructors, or same-spelling owner ambiguity.
- Broad declaration or backend rewrites are mixed into this idea while the
  parser owner probe still depends on rendered strings.
- Compatibility rendering remains the only route used by new owner-probe code
  without a removal condition.
- The exact old failure mode remains behind a new abstraction name:
  out-of-class owner classification still succeeds primarily because a
  rendered `qualified_owner` string matches.
