# Parser Out Of Class Owner Probe Token Sequence

Status: Draft
Created: 2026-05-06

Parent Ideas:
- `ideas/closed/139_parser_sema_rendered_string_lookup_removal.md`
- `ideas/open/145_move_record_tag_authority_from_parser_to_sema.md`

## Goal

Remove rendered-string authority from parser out-of-class member and
constructor owner probing.

The parser should detect lexical owners from token `TextId` sequences and
structured qualified-owner keys before deciding whether a declaration denotes
an out-of-class member, constructor, destructor, conversion, or operator
definition. A rendered owner string may remain for diagnostics and display,
but should not be the decision key.

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
on formatting a qualified owner and then interpreting that text.

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
- Tests are weakened, marked unsupported, or narrowed to avoid nested owners,
  operators, constructors, or same-spelling owner ambiguity.
- Broad declaration or backend rewrites are mixed into this idea while the
  parser owner probe still depends on rendered strings.
- Compatibility rendering remains the only route used by new owner-probe code
  without a removal condition.
- The exact old failure mode remains behind a new abstraction name:
  out-of-class owner classification still succeeds primarily because a
  rendered `qualified_owner` string matches.
