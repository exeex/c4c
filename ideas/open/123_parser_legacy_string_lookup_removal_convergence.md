# Parser Legacy String Lookup Removal Convergence

Status: Open
Created: 2026-04-29

Parent Ideas:
- [98_parser_sema_post_cleanup_structured_identity_leftovers.md](/workspaces/c4c/ideas/closed/98_parser_sema_post_cleanup_structured_identity_leftovers.md)

## Goal

Converge `src/frontend/parser` away from legacy `std::string` lookup maps.
Parser-owned text lookup should use `TextId` / `TextTable` keys where the text
is already interned, and parser semantic lookup should use a separate semantic
table or typed key rather than treating rendered text as meaning.

Legacy strings may remain as source spelling, diagnostics, compatibility input,
or final rendering data. They should not remain the key for ordinary parser
lookup when a `TextId` or domain-specific semantic key exists.

## Why This Idea Exists

The next legacy-removal pass needs to be split by module because cross-module
contracts can conflict. Recent BIR/HIR work exposed exactly that failure mode:
one test expected semantic carriers to canonicalize through `LinkNameId`, while
another test still protected drifted legacy spelling as identity.

Parser cleanup should use the same judgment standard as the rest of the legacy
string lookup removal:

- `TextId` is text identity only; it is not semantic authority
- `std::unordered_map<std::string, T>` text lookup should become
  `std::unordered_map<TextId, T>` where the text is interned
- semantic lookup must live in a domain table or typed semantic key, possibly
  indexed by `TextId`
- legacy strings are display, compatibility, selector input, diagnostics, or
  final spelling unless explicitly classified otherwise
- tests that protect legacy-string lookup authority should be updated when the
  module policy changes
- if a conflict reveals a separate initiative, create a new idea instead of
  silently expanding this one

## Scope

- Inventory parser-owned `std::unordered_map<std::string, ...>`, string lookup,
  map-key, and helper fallback paths under `src/frontend/parser`.
- Convert pure text lookup maps to `TextId` keys where the parser already has
  interned text.
- Split semantic lookup disguised as string lookup into parser/domain semantic
  tables or typed keys.
- Demote parser-owned string helper overloads that are now compatibility
  bridges rather than semantic authority.
- Update parser tests that currently assert legacy rendered-name precedence
  when the new contract says `TextId` text lookup or domain semantic lookup
  wins.
- Preserve strings required for diagnostics, source spelling, user-facing
  messages, final emitted text, or downstream compatibility bridges.

## Out Of Scope

- HIR symbol-table migration.
- LIR or BIR lookup authority changes.
- Broad type identity redesign.
- Removing strings that are only display or diagnostic data.
- Weakening supported behavior or marking tests unsupported to make cleanup
  pass.

## Conflict Policy

When parser changes collide with HIR, LIR, or BIR expectations, apply the shared
contract first:

- if the old test protects legacy string lookup as semantic authority, update
  the test to `TextId` lookup or domain semantic-table expectations
- if both sides need different real contracts, write a follow-up idea under
  `ideas/open/` and keep this idea focused
- do not add testcase-shaped shortcuts to satisfy one narrow fixture

## Acceptance Criteria

- Parser-owned pure text maps use `TextId` keys where interned text is
  available.
- Parser-owned semantic lookups use domain semantic tables or typed keys rather
  than rendered `std::string` keys.
- Remaining parser string lookups are classified as display, compatibility,
  diagnostic, final spelling, or unresolved downstream bridge.
- Focused parser tests prove legacy rendered strings do not override covered
  `TextId` or semantic-table lookup paths.
- Any unresolved cross-module conflict is captured as a separate open idea.

## Lifecycle Note: 2026-04-29

Active execution parked on `Step 3: Split Parser Semantic Lookup From Text
Spelling` after inventorying `DefinitionState::struct_tag_def_map`.

The first semantic-record family cannot be converted honestly inside
parser-owned files because the lookup input crosses through `TypeSpec`, and
`TypeSpec` carries record identity only as `const char* tag`. Converting
`struct_tag_def_map` to another string-like key would still leave rendered tag
spelling as semantic authority.

Follow-up bridge initiative, now closed:
`ideas/closed/127_typed_parser_record_identity_bridge.md`.

## Lifecycle Note: 2026-04-29 Return From Bridge

The typed parser record identity bridge is complete and closed at
`ideas/closed/127_typed_parser_record_identity_bridge.md`.

The parent parser cleanup can resume with `TypeSpec::record_def` carrying
parser semantic record identity where available. `TypeSpec::tag` remains
spelling, diagnostics, emitted text, and compatibility payload.
`DefinitionState::struct_tag_def_map` should now be treated as a
compatibility/final-spelling mirror, testing hook, and tag-only fallback rather
than the primary semantic record authority for converted parser paths.
