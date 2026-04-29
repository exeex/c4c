# Parser Legacy String Lookup Removal Convergence Runbook

Status: Active
Source Idea: ideas/open/123_parser_legacy_string_lookup_removal_convergence.md
Activated From: ideas/open/123_parser_legacy_string_lookup_removal_convergence.md
Returned From: ideas/closed/127_typed_parser_record_identity_bridge.md on 2026-04-29

## Purpose

Converge `src/frontend/parser` away from legacy rendered-string lookup
authority while preserving strings that are still needed for spelling,
diagnostics, compatibility, or final emitted text.

## Goal

Convert parser-owned pure text lookup to `TextId` where interned text is
available, and keep parser semantic lookup on typed/domain identity rather than
rendered `std::string` keys.

## Core Rule

Do not replace one rendered-string semantic authority path with another.
Classify each remaining string lookup before converting it: text lookup,
semantic lookup, compatibility fallback, diagnostic/source spelling, or final
rendering.

## Read First

- `ideas/open/123_parser_legacy_string_lookup_removal_convergence.md`
- `ideas/closed/127_typed_parser_record_identity_bridge.md`
- `todo.md`
- Parser state and helper maps under `src/frontend/parser/impl/`
- Parser tests that assert stale rendered spelling behavior

## Current Targets

- Parser-owned `std::unordered_map<std::string, ...>` maps and helper fallback
  paths.
- Pure text lookup surfaces where the parser already has `TextId` or can
  cheaply intern the lookup text.
- Semantic lookup surfaces that should use parser-domain identity instead of
  rendered strings.
- Compatibility mirrors and final-spelling maps that should remain explicit.

## Non-Goals

- Do not revisit the typed parser record identity bridge except to preserve its
  contract.
- Do not remove `TypeSpec::tag` or `DefinitionState::struct_tag_def_map`
  compatibility/final-spelling behavior.
- Do not change HIR, LIR, or BIR lookup authority in this runbook.
- Do not remove strings used only for diagnostics, source spelling, user-facing
  messages, or final emitted text.
- Do not downgrade supported tests or mark covered behavior unsupported.

## Working Model

- `TextId` is parser text identity, not semantic authority by itself.
- Parser semantic lookup should use a domain key, typed payload, record/node
  identity, or explicit semantic table.
- Legacy strings may remain as compatibility input, display data, diagnostics,
  source spelling, testing hooks, or final rendered keys.
- The closed typed record bridge established that `TypeSpec::record_def` is the
  parser semantic record identity when available, while
  `struct_tag_def_map` is a compatibility/final-spelling mirror.

## Execution Rules

- Start by re-inventorying remaining parser string maps after the record bridge.
- Keep each packet bounded to one map family or helper family.
- Prefer existing parser `TextTable`, `TextId`, typed AST payloads, and domain
  tables over new broad abstractions.
- Add or update tests only when they prove rendered-string authority no longer
  wins for the converted path.
- Preserve compatibility fallbacks explicitly where external or final-spelling
  behavior still depends on rendered strings.
- Pair code-changing packets with fresh build or focused parser/frontend proof
  selected by the supervisor.

## Ordered Steps

### Step 1: Re-Inventory Parser String Lookup After Record Bridge

Goal: classify the remaining parser string lookup surfaces with
`struct_tag_def_map` treated as a compatibility/final-spelling mirror.

Primary targets:
- Parser-owned `std::unordered_map<std::string, ...>` declarations.
- String-keyed helper lookups under `src/frontend/parser/impl/`.
- Tests that still assert rendered spelling as lookup authority.
- `DefinitionState::struct_tag_def_map` sites only as compatibility context.

Concrete actions:
- Inventory each remaining string map as pure text lookup, semantic lookup,
  compatibility fallback, diagnostic/source spelling, final rendering, or
  unresolved downstream boundary.
- Identify the next smallest conversion packet that does not depend on
  `struct_tag_def_map` semantic authority.
- Record any separate downstream bridge as a new open idea instead of widening
  this runbook.

Completion check:
- `todo.md` names the remaining classified map families, the first bounded
  conversion packet, retained compatibility categories, and the focused proof
  command.

### Step 2: Convert Pure Parser Text Lookup To TextId

Goal: move parser-owned text lookup maps to `TextId` keys where the parser
already has interned text or can intern at the boundary.

Primary targets:
- Local parser maps keyed by source spelling or identifier text.
- Helpers that receive parser-owned text and immediately perform string map
  lookup.

Concrete actions:
- Convert one pure text map family at a time.
- Intern lookup text at the boundary where needed.
- Keep rendered strings as spelling/diagnostic payloads, not map authority.
- Add focused tests or update existing assertions to prove equivalent lookup
  behavior.

Completion check:
- Converted pure text lookup maps use `TextId` keys, retained strings are
  spelling or diagnostics, and focused parser tests pass.

### Step 3: Split Remaining Parser Semantic Lookup From Text Spelling

Goal: remove rendered-string authority from remaining parser-owned semantic
lookup paths.

Primary targets:
- String-keyed maps whose values represent semantic declarations, records,
  overload sets, typedef ownership, template entities, or domain objects.
- Helper overloads that hide a semantic lookup behind a `std::string` key.

Concrete actions:
- For each semantic family, choose an existing typed/domain identity or add the
  smallest parser-local table that represents meaning directly.
- Preserve rendered-string fallback only where compatibility or final spelling
  requires it.
- Add stale-rendered-spelling tests for converted semantic paths.
- If a family needs a separate bridge, create a focused idea under
  `ideas/open/` and request lifecycle routing.

Completion check:
- Covered semantic lookup paths no longer use rendered strings as primary
  authority, and tests prove stale rendered strings cannot override the typed
  or domain identity.

### Step 4: Demote Compatibility String Helpers And Tests

Goal: make retained string paths visibly compatibility, diagnostic, or final
rendering support rather than ordinary parser lookup authority.

Primary targets:
- Helper names, comments, and tests around retained string fallbacks.
- Parser tests that previously treated legacy rendered strings as primary
  authority.

Concrete actions:
- Rename, narrow, or document retained helpers where the current name hides
  compatibility/final-spelling behavior.
- Keep compatibility coverage but assert fallback behavior explicitly.
- Avoid deleting strings that still serve diagnostics, source spelling, or
  final emitted text.

Completion check:
- Remaining parser string lookup sites are discoverably classified, and tests
  distinguish compatibility fallback from typed or `TextId` authority.

### Step 5: Reprove Parser Cleanup And Decide Closure Or Split

Goal: validate the parser cleanup route and decide whether source idea 123 is
complete or should split remaining work.

Primary targets:
- Focused parser/frontend tests changed during this runbook.
- Broader parser-adjacent validation selected by the supervisor.
- Any unresolved cross-module boundary found during execution.

Concrete actions:
- Run focused proofs for converted parser lookup families.
- Run broader frontend validation appropriate for parser lookup changes.
- Review expectation updates for unsupported downgrades or testcase-shaped
  shortcuts.
- Close idea 123 only if all acceptance criteria are satisfied; otherwise park
  or split remaining durable work into `ideas/open/`.

Completion check:
- Regression guard passes for the selected scope, no overfit or downgrade is
  present, and lifecycle state clearly records closure, parking, or split.
