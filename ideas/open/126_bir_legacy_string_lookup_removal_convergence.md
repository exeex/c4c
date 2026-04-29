# BIR Legacy String Lookup Removal Convergence

Status: Open
Created: 2026-04-29

Parent Ideas:
- [122_bir_string_legacy_path_cleanup.md](/workspaces/c4c/ideas/closed/122_bir_string_legacy_path_cleanup.md)

## Goal

Continue converging `src/backend/bir` away from legacy `std::string` lookup
maps after the first BIR string cleanup, using BIR tests as the primary
contract for separating text identity from semantic identity.

The intended direction is that ordinary BIR text lookup uses `TextId` /
`TextTable` and ordinary BIR semantic lookup uses typed records or domain
semantic IDs such as `LinkNameId` where available. Legacy strings may remain
for display, selectors, dumps, diagnostics, final spelling, compatibility
payloads, or unresolved upstream boundaries, but not as primary lookup
authority.

## Why This Idea Exists

The recent LinkNameId cleanup exposed a direct HIR-vs-BIR test contract
conflict: HIR expected BIR names/callees to canonicalize through semantic IDs,
while an older BIR test expected drifted legacy display spelling to remain the
BIR identity. The correct next step is to make BIR tests enforce the same
authority policy as production cleanup.

Shared standard:

- `TextId` is text identity only; it is not semantic authority
- `std::unordered_map<std::string, T>` text lookup should become
  `std::unordered_map<TextId, T>` where the text is interned
- typed BIR fields, `LinkNameId`, block label IDs, structured type IDs, and
  domain tables are semantic authority when present
- raw strings are retained only when explicitly classified
- test contracts may be changed when they preserve legacy lookup authority
- if a conflict reveals missing HIR/LIR metadata, write a new idea rather than
  expanding this BIR route

## Scope

- Inventory remaining BIR `std::unordered_map<std::string, ...>`, string
  lookup, and verifier matching paths in `src/backend/bir`.
- Convert pure text lookup maps to `TextId` keys where BIR has interned text.
- Prefer `LinkNameId`, block label IDs, structured type IDs, typed records, and
  prepared metadata over raw string comparison for semantic lookup.
- Update BIR verifier and printer tests so semantic identity expectations are
  structured-first.
- Preserve dump readability and selector input while keeping them classified as
  non-authority boundaries.
- Add mismatch tests where both structured identity and legacy string spelling
  are present and disagree.

## Out Of Scope

- Parser, HIR, or LIR producer migration except as separately opened blockers.
- MIR or target assembly rewrites.
- Removing dump/final spelling text that is not used for semantic lookup.
- Weakening backend expectations or marking supported paths unsupported.
- Named-case shortcuts that only satisfy one fixture.

## Conflict Policy

When BIR work conflicts with upstream or downstream tests:

- if BIR has `TextId` for text lookup or a domain semantic identity, update
  tests to expect that identity to win over drifted legacy spelling
- if strings are required for output, classify them as display/final spelling
  and keep semantic lookup separate
- if the data needed for structured identity is absent upstream, open a new
  HIR or LIR idea instead of adding BIR fallback authority

## Acceptance Criteria

- Covered BIR pure text lookup paths use `TextId` keys.
- Covered BIR semantic lookup paths use domain IDs, typed records, or semantic
  tables before raw string lookup.
- BIR tests reject drifted/stale legacy strings as lookup authority where
  `TextId` or semantic identity is available.
- Remaining BIR string fields are documented as display, selector,
  compatibility, final spelling, diagnostics, or unresolved boundary.
- Focused BIR and LIR-to-BIR tests pass with no expectation downgrades.
- Any cross-module blocker becomes a separate idea under `ideas/open/`.
