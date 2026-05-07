# Qualified Name Deferred Carrier Authority

Status: Closed
Created: 2026-05-07
Closed: 2026-05-07

Parent Ideas:
- `ideas/closed/139_parser_sema_rendered_string_lookup_removal.md`
- `ideas/closed/145_move_record_tag_authority_from_parser_to_sema.md`

## Goal

Stop treating rendered qualified-name strings such as `A::B::C` as semantic
qualified-name authority.

Qualified names may be resolved eagerly in Sema when enough information is
available, or deferred into HIR when template/dependent context prevents a final
answer. In both cases the carrier must remain structured: token `TextId`
sequence, qualifier path, namespace/global metadata, lookup-domain expectation,
and any substitution/dependent context needed for late resolution.

The important boundary is not "Sema must resolve everything before HIR." The
real boundary is: **Parser, Sema, and HIR must not flatten a qualified name into
one rendered spelling and later split that spelling to recover semantic
identity.**

## Working Responsibility Split

### Parser

Parser owns syntax recognition and provisional carriers.

Parser should:

- parse `A::B::C` from the source token stream into a structured carrier such as
  `QualifiedNameRef` / `QualifiedNameKey`
- preserve base `TextId`, qualifier `TextId` sequence, `is_global_qualified`,
  namespace/context hints, and source spelling for diagnostics/display
- perform only grammar-required disambiguation, such as parse-time typedef-name
  decisions

Parser should not:

- intern `A::B::C` as one semantic `TextId` and split it later
- decide final typedef/tag/value/function identity
- use string compatibility maps as semantic lookup authority

### Sema

Sema owns eager semantic lookup when the answer is available.

Sema should:

- resolve non-dependent qualified names through domain tables
- distinguish typedef, record/tag, value, function, namespace, concept, and
  template identity
- apply scope, namespace, using/alias, and incomplete-to-complete rules
- produce a deferred semantic carrier when dependent/template context prevents a
  final answer

Sema output may therefore be either:

- a resolved domain identity, or
- a deferred lookup carrier with structured name metadata and domain expectation

### HIR

HIR may perform late semantic resolution for deferred/template-dependent cases.

HIR should:

- consume resolved Sema identities when available
- consume Sema deferred carriers when late template substitution is required
- complete deferred lookup using structured owner/name/domain/substitution
  metadata
- convert the result into HIR/module-domain keys for lowering

HIR should not:

- split rendered qualified-name strings
- use one `TextId` containing `::` as compound semantic identity
- rediscover typedef/tag/value/function identity from display spelling
- treat parser compatibility helpers as late semantic lookup authority

In short: HIR may do late Sema work, but it must not do string-based Sema work.

## Why This Idea Exists

The current parser already has some correct structured qualified-name machinery:
`peek_qualified_name()` and `parse_qualified_name()` build `QualifiedNameRef`
from token order, and `qualified_name_key()` interns qualifier `TextId`
sequences into `NamePathTable`.

The remaining problem is the compatibility escape hatch. Current anchors
include:

- `src/frontend/parser/impl/core.cpp`
  - `qualified_key_in_context()`
  - `find_compatibility_key_from_rendered_qualified_spelling()`
  - `intern_compatibility_key_from_rendered_qualified_spelling()`
  - `has_typedef_type(TextId)` / `find_typedef_type(TextId)` when the spelling
    contains `::`
  - `register_known_fn_name_in_context()` compatibility registration
- `src/shared/qualified_name_table.hpp`
  - `split_qualified_name_scope()`
  - `qualified_name_base_matches()`

Those paths can still flatten `A::B::C` into a rendered spelling or one `TextId`
and later recover structure by string splitting. That violates the policy from
139 through 145:

- Lexer interns spelling into `TextId`.
- `TextId` has no semantic meaning beyond spelling identity.
- Compound semantic names live in domain tables keyed by structured components.
- Deferred lookup is allowed, but the deferred payload must remain structured.

## In Scope

- Inventory every parser/Sema/HIR path that accepts a single `TextId` whose
  spelling may contain `::` and uses it for semantic lookup.
- Split those APIs into:
  - unqualified `TextId` APIs that reject or ignore `::`
  - structured `QualifiedNameRef` / `QualifiedNameKey` APIs for compound names
- Remove or demote parser semantic reliance on
  `find_compatibility_key_from_rendered_qualified_spelling()`.
- Classify `split_qualified_name_scope()` and related helpers as display-only,
  compatibility-only, or removable.
- Ensure dependent/template cases can carry unresolved qualified names as
  structured deferred carriers into HIR.
- Add tests where a rendered/suffix split route would choose the wrong name but
  token-sequence/domain-key lookup behaves correctly.
- Document any remaining rendered-qualified-name compatibility path with its
  owner, reason, and removal condition.

## Out Of Scope

- Forcing all C++ name lookup to finish in Sema before HIR.
- Replacing every diagnostic, dump, or display spelling.
- Broad HIR/LIR/BIR/backend rewrites beyond narrow carrier plumbing needed to
  preserve structured deferred lookup.
- Changing C++ lookup rules beyond replacing rendered-string authority with
  structured carriers.
- Treating one interned `TextId` for `A::B::C` as a valid semantic qualified
  name.
- Weakening tests, marking qualified-name cases unsupported, or using
  expectation-only proof as progress.

## Acceptance Criteria

- Parser qualified-name construction produces `QualifiedNameRef` /
  `QualifiedNameKey` from source token `TextId` sequence without flattening and
  reparsing rendered spelling.
- Semantic APIs that take `TextId` are clearly unqualified-name APIs and do not
  use spellings containing `::` as compound-name authority.
- Qualified semantic lookup uses domain-specific structured keys, or produces a
  structured deferred carrier when lookup must wait for HIR/template
  substitution.
- HIR late lookup, where still required, consumes structured deferred carriers
  rather than rendered qualified strings.
- `find_compatibility_key_from_rendered_qualified_spelling()` and
  `split_qualified_name_scope()` are either removed from semantic paths or left
  as explicitly bounded compatibility/display helpers.
- Tests prove at least one collision or stale-route case where rendered
  splitting would be wrong.

## Closure Notes

The active runbook completed Steps 1-8 and fenced the remaining rendered
qualified-name helpers as compatibility/display boundaries rather than primary
semantic lookup authority. `find_compatibility_key_from_rendered_qualified_spelling()`
is retained only for legacy rendered qualified `TextId` carriers after
structured metadata has been tried, and `split_qualified_name_scope()` is
documented as a legacy rendered-spelling helper.

Full deletion of those compatibility bridges is intentionally split to
`ideas/open/147_rendered_qualified_compatibility_bridge_removal.md`, because
removal currently regresses still-migrating qualified template/HIR paths and
needs its own structured-carrier migration packet.

## Reviewer Reject Signals

- A slice claims progress by renaming a string helper while still resolving
  qualified names by rendering `A::B::C` and splitting it later.
- A route treats one `TextId` containing `A::B::C` as equivalent to
  `{A, B, C}` in a structured qualified-name key.
- HIR late lookup is implemented by splitting rendered strings instead of
  consuming a Sema/parser structured deferred carrier.
- Compatibility helpers remain the primary semantic lookup path after the slice
  claims authority moved.
- Tests are weakened, marked unsupported, or shaped around one named fixture.
- The implementation adds named-case shortcuts instead of building or consuming
  token-sequence/domain keys.
