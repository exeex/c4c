# Parser Out Of Class Owner Probe Token Sequence

Status: Open
Created: 2026-05-06
Opened: 2026-05-08

Parent Ideas:
- `ideas/closed/139_parser_sema_rendered_string_lookup_removal.md`
- `ideas/closed/145_move_record_tag_authority_from_parser_to_sema.md`
- `ideas/closed/146_qualified_name_deferred_carrier_authority.md`
- `ideas/closed/147_rendered_qualified_compatibility_bridge_removal.md`
- `ideas/closed/148_hir_static_member_carrier_authority_decomposition.md`
- `ideas/closed/149_template_instantiation_structured_argument_key.md`
- `ideas/closed/150_nttp_type_binding_domain_key_contract.md`

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

## Current Parser `token_spelling` Boundary

Parser still has many `token_spelling(...)` calls. This idea does not require
removing every spelling access. The boundary is whether the spelling is source
syntax/display or semantic authority.

Reasonable parser spelling uses:

- source-literal and token-lexeme handling, such as integer parsing, string
  literal concatenation, language linkage strings, and pragma payloads
- keyword-like extension syntax that does not have a dedicated token kind in
  the current lexer, such as `__label__`, `_Nullable`, `throw`,
  `__underlying_type`, or identifier-spelled `noexcept`
- diagnostics, debug output, and AST legacy `char* name` mirrors, when a
  `TextId` or structured key is also preserved for semantic consumers
- temporary compatibility wrappers that are explicitly gated by structured
  metadata miss behavior and have removal conditions

Suspicious parser spelling uses:

- rendered owner strings used to decide declaration owner shape
- spelling comparison used to classify constructors, destructors, conversion
  functions, operators, or member ownership
- compound paths assembled as `A::B::C` strings before a structured
  `TextId` segment path / `QualifiedNameKey` is produced
- spelling-derived names that are later used as semantic lookup keys without a
  domain table, owner key, or parameter/member index

The 151 target is the suspicious owner-probe family, not every spelling access
in parser.

## Current Anchors To Inspect By Name

- `probe_special_member_owner` in
  `src/frontend/parser/impl/declarations.cpp` builds an `owner` string while
  probing out-of-class special-member declarations. This should become a
  structured probe result based on token `TextId` segments, not a rendered
  owner string.
- `consume_special_member_owner` in
  `src/frontend/parser/impl/declarations.cpp` already collects
  `out_owner_text_ids`, but it also keeps `out_owner_segments` and still uses
  `token_spelling(parser.peek(1)) == seg` for constructor detection. The
  constructor check should compare token/TextId components directly.
- The out-of-class constructor path near the `ctor_name` handling in
  `src/frontend/parser/impl/declarations.cpp` constructs
  `qualified_ctor_name` from `qualified_owner` plus rendered `ctor_name`.
  That string may remain as display, but it should not be the owner or
  constructor identity key.
- Function-pointer declarator branches in
  `src/frontend/parser/impl/declarations.cpp` use `token_spelling` for
  nullability annotations and `decl_name` mirrors. Those are syntax and legacy
  name-storage uses; they should not be pulled into this idea unless they feed
  owner identity.
- Parser template fallback in
  `src/frontend/parser/impl/types/template.cpp` still compares
  `token_spelling(tok)` with legacy binding spelling when structured metadata
  is absent. Idea 150 classified this as compatibility, so 151 should not
  reopen it except to ensure owner-probe work does not add new spelling
  authority.
- Qualified-name parsing helpers in `src/frontend/parser/impl/core.cpp` may
  keep rendered `base_name` mirrors when they also produce `base_text_id` and
  qualifier `TextId` segments. 151 should use these structured carriers
  instead of flattening owners back into strings.
- Expression paths such as qualified member spelling and `__builtin_offsetof`
  field paths may need later cleanup if they become semantic lookup keys, but
  they are not the primary 151 target unless they share the out-of-class owner
  probe route.

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
