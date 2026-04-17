# Parser And HIR Text-Id Convergence Runbook

Status: Active
Source Idea: ideas/open/15_parser_and_hir_text_id_convergence.md
Activated from: ideas/open/15_parser_and_hir_text_id_convergence.md

## Purpose

Turn the remaining parser/HIR persistent text carriers from owned strings into
bounded `TextId` paths without collapsing `TextId`, `SymbolId`, and
`LinkNameId`.

## Goal

Land one narrow parser-to-HIR path where persistent textual identity is stored
as `TextId` instead of only `std::string`, with focused proof in parser and HIR
tests.

## Core Rule

Migrate only stable TU-scoped text identity. Do not route source-atom identity
through `TextId`, do not route final emitted names away from `LinkNameId`, and
do not rewrite diagnostic or serialization strings into ids.

## Read First

- [ideas/open/15_parser_and_hir_text_id_convergence.md](/workspaces/c4c/ideas/open/15_parser_and_hir_text_id_convergence.md)
- [src/frontend/string_id_table.hpp](/workspaces/c4c/src/frontend/string_id_table.hpp)
- [src/frontend/parser/parser.hpp](/workspaces/c4c/src/frontend/parser/parser.hpp)
- [src/frontend/hir/hir_ir.hpp](/workspaces/c4c/src/frontend/hir/hir_ir.hpp)

## Current Targets

- Keep the existing parser-side `QualifiedNameRef` and persistent parser text-id
  work as the starting point rather than reopening that slice.
- Move at least one bounded HIR text carrier from persistent string storage to
  `TextId`.
- Prove one parser-to-HIR route now preserves `TextId` identity end to end.

## Non-Goals

- Replacing every parser/HIR string field in one pass.
- Changing final link-visible naming policy or `LinkNameId` ownership.
- Converting diagnostics, debug summaries, specialization keys, or inline-asm
  text into ids.
- Touching unrelated lifecycle or implementation work outside this idea.

## Working Model

- `TextId`: TU-scoped text identity.
- `SymbolId`: source-atom identity for parser/source-facing semantics.
- `LinkNameId`: final emitted logical symbol identity.
- Parallel fields are acceptable for the first landing when needed to keep the
  route mechanical and easy to validate.

## Execution Rules

- Prefer one bounded HIR carrier and its nearest parser/HIR construction path
  over broad field churn.
- If a registry or map still needs a temporary string key for untouched paths,
  keep it until the chosen route is proven.
- Put execution progress in `todo.md`; do not rewrite this runbook for routine
  packet updates.
- Validation ladder for each packet: build, targeted parser/HIR tests, then
  broaden only if the blast radius grows.

## Step 1. Choose The Narrow End-To-End Route

Goal: Pick the smallest parser-to-HIR path that still satisfies the idea's
acceptance gap.

Primary targets:
- one persistent parser carrier already using `TextId`
- one HIR carrier that still stores stable TU text as `std::string`

Actions:
- inspect the existing parser text-id carriers and find the nearest HIR sink
  that still falls back to string ownership
- choose a route that can be proven in both parser and HIR tests without
  broad registry rewrites
- record any nearby non-scope strings that must remain strings

Completion check:
- one bounded parser-to-HIR migration path is named and justified before code
  edits start

## Step 2. Thread `TextId` Through The Chosen HIR Carrier

Goal: Make the selected HIR carrier persist TU text identity as `TextId`.

Primary targets:
- the chosen HIR IR field(s)
- the nearest builder/lowering helpers that populate those fields

Actions:
- add a `TextId` field or migrate the existing field when the route is local and
  low-risk
- thread `TextTable` access through the relevant parser/HIR construction helpers
- preserve any required legacy string field only when untouched consumers still
  need it

Completion check:
- the chosen HIR carrier is no longer string-only on the selected route

## Step 3. Keep Id Spaces Separate At Consumer Boundaries

Goal: Prevent route drift into symbol-id or link-name-id collapse.

Actions:
- verify the chosen parser/HIR path keeps source-atom semantics on `SymbolId`
  and emitted names on `LinkNameId`
- leave diagnostic/debug/serialization strings unchanged
- avoid testcase-shaped one-off storage shortcuts

Completion check:
- the migrated route has a clear `TextId` justification and no id-space mixing

## Step 4. Prove The Route

Goal: Add focused proof that the new path round-trips correctly.

Actions:
- add or update one parser test for the persistent parser carrier if needed
- add or update one HIR-facing test proving the migrated carrier now preserves
  `TextId` identity
- run `frontend_parser_tests` and `frontend_hir_tests`

Completion check:
- targeted parser and HIR proof is green and directly covers the migrated path
