# Parser Legacy String Lookup Removal Runbook

Status: Active
Source Idea: ideas/open/123_parser_legacy_string_lookup_removal_convergence.md
Activated From: ideas/open/123_parser_legacy_string_lookup_removal_convergence.md

## Purpose

Converge `src/frontend/parser` away from legacy `std::string` lookup
authority, while preserving strings that are spelling, diagnostics,
compatibility input, or final rendering data.

## Goal

Make parser-owned text lookup use `TextId` where text is already interned, and
move parser-owned semantic lookup into domain tables or typed semantic keys
instead of rendered string keys.

## Core Rule

Do not prove this cleanup by weakening supported behavior, marking tests
unsupported, or adding named-case shortcuts. Classify retained strings and
prefer semantic lowering/generalization over testcase-shaped matching.

## Read First

- `ideas/open/123_parser_legacy_string_lookup_removal_convergence.md`
- Parser-owned symbol, declaration, helper, and lookup code under
  `src/frontend/parser`.
- Parser tests that exercise names, selectors, declarations, semantic lookup,
  diagnostics, and drifted spelling behavior.
- Nearby HIR boundary code only when parser changes require confirming a
  downstream compatibility bridge; do not start HIR cleanup in this plan.

## Current Targets

- Parser-owned `std::unordered_map<std::string, ...>` and equivalent rendered
  string-keyed containers.
- Parser lookup helpers and overloads that treat rendered text as authority.
- Parser semantic lookup paths where a domain table or typed key should own
  meaning.
- Tests that currently assert legacy rendered-name precedence where the new
  parser contract says `TextId` or domain semantic lookup should win.

## Non-Goals

- Do not perform HIR symbol-table migration.
- Do not change LIR or BIR lookup authority.
- Do not redesign broad type identity.
- Do not remove strings that are only diagnostics, source spelling,
  user-facing messages, final emitted text, or compatibility bridge payloads.
- Do not downgrade supported tests or replace semantic checks with
  string-shaped shortcuts.

## Working Model

- `TextId` is text identity only; it is not semantic authority.
- Pure parser text lookup should use `TextId` keys when the parser already has
  interned text.
- Parser semantic lookup should be represented by a domain table or typed key,
  possibly indexed by `TextId`, rather than by rendered `std::string`.
- Retained strings must be discoverably classified as display, diagnostics,
  source spelling, compatibility, final rendering, or unresolved downstream
  bridge data.

## Execution Rules

- Start with inventory and classification before changing lookup
  representation.
- Keep each implementation packet bounded to one parser-owned lookup family.
- Prefer existing parser data structures, `TextTable`, and domain records over
  new broad abstractions.
- Update tests only when they currently protect legacy string lookup authority
  rather than supported parser semantics.
- If a required change belongs to HIR, LIR, or BIR, record a blocker or create
  a separate open idea instead of expanding this plan.
- Pair code-changing packets with fresh build or focused parser test proof
  selected by the supervisor.

## Ordered Steps

### Step 1: Inventory Parser String Authority

Goal: map parser-owned string lookup uses and classify which are lookup
authority versus retained non-authority strings.

Primary targets:
- Parser lookup maps, helper overloads, semantic tables, and fallback paths.
- Parser tests that exercise rendered names, selectors, declarations, and
  diagnostics.
- HIR-facing parser boundary only where needed to classify a compatibility
  bridge.

Concrete actions:
- Inspect parser-owned `std::unordered_map<std::string, ...>` and equivalent
  string-keyed lookup surfaces.
- Classify each string use as source spelling, diagnostics, compatibility
  input, final rendering, pure text lookup, semantic lookup, or unresolved
  downstream bridge.
- Identify where interned `TextId` values or existing parser domain records are
  already available.
- Record the first safe implementation packet and proof command in `todo.md`
  when execution begins.

Completion check:
- `todo.md` names the first bounded parser lookup family, its retained string
  categories, and the focused proof command for implementation.

### Step 2: Convert Pure Parser Text Lookup

Goal: replace pure parser text lookup maps with `TextId` keys where interned
text is already available.

Primary targets:
- Parser maps and helpers whose key is text identity, not semantic authority.
- Parser call sites that already carry or can cheaply obtain the relevant
  `TextId`.
- Focused parser tests for drifted rendered spelling versus interned text
  lookup.

Concrete actions:
- Convert one pure text lookup family at a time from rendered string keys to
  `TextId` keys.
- Keep source spelling and diagnostics available as payloads where callers
  need them.
- Preserve compatibility string overloads only as explicit bridges that intern
  or translate input before lookup.
- Add or update focused tests that show rendered spelling does not override
  the covered `TextId` lookup path.
- Treat parser-template rendered mirrors as the completed Step 2 proof family:
  structured `QualifiedNameKey` / `NttpDefaultExprKey` lookups stay primary,
  and rendered maps remain compatibility or final-spelling mirrors only.
- Do not include `struct_tag_def_map` in this step. Record tags feed parser
  semantic record lookup and downstream `TypeSpec::tag` bridges, so they belong
  to Step 3 or to a separate bridge blocker if they cannot be contained in the
  parser.

Completion check:
- Covered pure text lookups use `TextId` or structured text keys, focused
  parser tests prove the intended lookup behavior without output or diagnostic
  regressions, and any remaining string-keyed parser family is classified as
  semantic lookup, compatibility/final spelling, diagnostics, or a downstream
  bridge.

### Step 3: Split Parser Semantic Lookup From Text Spelling

Goal: move parser semantic lookup disguised as rendered string lookup into a
domain semantic table or typed key.

Primary targets:
- First packet: the parser record-tag bridge around
  `DefinitionState::struct_tag_def_map`, `TypeSpec::tag`, `eval_const_int`,
  `offsetof`, `sizeof` / `alignof`, and template-instantiated struct records.
- Parser semantic lookup paths for declarations, symbols, selectors, or
  domain-specific parser records.
- Helpers that currently make rendered strings the authority for meaning.
- Tests that assert legacy rendered-name precedence over semantic identity.

Concrete actions:
- Start with a semantic-record packet for `struct_tag_def_map`: inventory each
  parser-owned read/write path, classify source spelling versus record identity,
  and identify where an existing `Node*`, record definition, tag declaration, or
  typed key can carry semantic authority.
- Preserve current `TypeSpec::tag` string behavior as an explicit bridge until
  the packet proves every affected parser path has a semantic record source;
  do not do a wholesale string-map deletion.
- Convert only the first bounded record-tag lookup surface that can be kept
  inside parser-owned files. If HIR/LIR/BIR contracts are required, stop and
  record a separate bridge blocker instead of expanding this plan.
- Replace one semantic string authority path at a time with a typed key,
  parser-domain table, or other existing semantic record.
- Use `TextId` only as text identity or table indexing support, not as final
  semantic authority.
- Keep diagnostics and source spelling separated from lookup authority.
- Update tests when they are enforcing legacy rendered-name semantics instead
  of the intended parser contract.

Completion check:
- The first record-tag semantic packet either converts a bounded parser-owned
  lookup surface to semantic record authority with focused parser proof, or
  records the exact downstream bridge blocker for a separate open idea.
  Covered semantic lookups resolve through parser-domain authority before raw
  spelling, with focused tests proving drifted rendered names cannot override
  the semantic table.

### Step 4: Demote Compatibility Helpers And Document Retained Strings

Goal: make remaining parser string helpers and fields explicit as
compatibility, display, diagnostic, final spelling, or unresolved bridge data.

Primary targets:
- Parser helper overloads that still accept `std::string`.
- Parser model comments or nearby documentation for retained string fields.
- Compatibility and diagnostic tests that intentionally keep strings.

Concrete actions:
- Rename, narrow, or document compatibility string overloads so they are not
  mistaken for primary lookup authority.
- Document retained string fields or helper paths near their definitions or in
  focused tests.
- Add or tighten tests that distinguish retained spelling from lookup
  authority.
- Record any unresolved cross-module conflict as a separate open idea if it is
  not required to finish parser cleanup.

Completion check:
- Remaining parser string lookup surfaces are discoverably classified, and
  tests distinguish retained spelling from text or semantic lookup authority.

### Step 5: Broader Reprove And Lifecycle Decision

Goal: prove the parser cleanup at the right breadth and decide whether the
source idea can close or needs another runbook revision.

Primary targets:
- Focused parser tests changed during this plan.
- Parser-adjacent frontend tests selected by the supervisor.
- Any follow-up idea required for HIR, LIR, or BIR boundary conflicts.

Concrete actions:
- Re-run the focused proof matrix for converted text and semantic lookup
  paths.
- Run broader frontend validation appropriate for parser lookup changes.
- Review expectation updates to ensure no supported behavior was weakened.
- Record unresolved downstream bridge work in `todo.md` or a separate open
  idea before asking for lifecycle closure.
- Ask the supervisor for completion review before closure.

Completion check:
- Focused and broader proof are recorded in `todo.md`, overfit risk is
  reviewed, retained parser string boundaries are documented, and the plan
  owner can decide whether the source idea is complete.
