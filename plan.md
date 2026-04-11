# Frontend Source-Atom Symbol Table And Id-Keyed Semantic Maps

Status: Active
Source Idea: ideas/open/06_frontend_source_atom_symbol_table_and_id_keyed_semantic_maps.md
Activated from: ideas/draft/06_frontend_source_atom_symbol_table_and_id_keyed_semantic_maps.md

## Purpose

Introduce split frontend identity layers so raw token text/file provenance and
parser-owned identifier symbols each get stable ids at the right boundary, then
migrate parser semantic tables from string keys toward id keys using
warning-driven convergence.

## Goal

Land a staged migration that:

- keeps `SymbolId` parser-owned
- gives `Token` stable `text_id` / `file_id`
- keeps `lexeme` / `file` as deprecated bridge fields
- uses compiler warnings to expose legacy call sites while parser tables move
  to `SymbolId`

## Core Rule

Treat `TextId` / `FileId` as token-layer storage identities and `SymbolId` as
parser/sema-level identifier identity. Do not treat `SymbolId` as a token
field, and do not collapse qualified names, mangled names, anonymous internal
names, or other synthesized strings into the same symbol model in this runbook.

## Read First

- [ideas/open/06_frontend_source_atom_symbol_table_and_id_keyed_semantic_maps.md](/workspaces/c4c/ideas/open/06_frontend_source_atom_symbol_table_and_id_keyed_semantic_maps.md)
- [src/frontend/lexer/token.hpp](/workspaces/c4c/src/frontend/lexer/token.hpp)
- [src/frontend/parser/parser.hpp](/workspaces/c4c/src/frontend/parser/parser.hpp)
- [src/frontend/parser/parser_core.cpp](/workspaces/c4c/src/frontend/parser/parser_core.cpp)
- [src/frontend/parser/parser_types_declarator.cpp](/workspaces/c4c/src/frontend/parser/parser_types_declarator.cpp)

## Current Targets

- shared string-id helpers and token-layer text/file ids in `src/frontend/*`
- lexer token construction in `src/frontend/lexer/*`
- parser-owned id types and symbol interning helpers in `src/frontend/parser/*`
- parser typedef and nearby identifier-keyed semantic tables in
  `src/frontend/parser/*`
- parser-local qualified-name and lookup helpers that currently traffic in raw
  identifier strings
- narrow validation for lexer/token shape, parser typedef behavior, and
  speculative parsing

## Non-Goals

- repo-wide replacement of `std::string`
- interning full qualified names such as `A::B::C`
- mangled-name or generated-name redesign
- backend symbol-table work
- snapshot/journaling redesign for tentative parsing

## Working Model

- `TextTable` and `FileTable` own frozen string/path storage
- `Token` carries `text_id` / `file_id` but keeps legacy `lexeme` / `file`
  during the migration window
- `SymbolId` is for identifier atoms only
- parser/sema assigns `SymbolId` values by interning identifier-token `TextId`
  values or spellings when semantic lookup/table mutation needs atom identity
- parser semantic facts stay stage-local even when their keys become ids
- deprecated token string fields remain as a compatibility bridge and compiler
  warning source until migrated call sites are retired

## Execution Rules

- keep the first landed slice small and behavior-preserving
- prefer parser-local wrappers over exposing raw container details further
- preserve diagnostics and tentative-parse behavior before chasing optimization
- migrate only tables whose keys are genuinely source identifier atoms
- use `[[deprecated]]` on token bridge fields to tighten migration via
  compiler warnings
- keep warning-driven convergence incremental; do not try to eliminate every
  legacy token string use in one slice
- validate the migration with targeted parser/frontend coverage before widening

## Step 1: Lock The Shared Id Storage Shapes

Goal: finalize the common id-table helper plus token-layer text/file ids so
later lexer/parser wiring shares one storage model.

Primary targets:
- [src/frontend/string_id_table.hpp](/workspaces/c4c/src/frontend/string_id_table.hpp)
- [src/frontend/lexer/token.hpp](/workspaces/c4c/src/frontend/lexer/token.hpp)
- [src/frontend/parser/parser.hpp](/workspaces/c4c/src/frontend/parser/parser.hpp)

Actions:
- keep one generic stable-id helper and derive `TextTable`, `FileTable`, and
  parser symbol-id backing from it
- keep `SymbolTable` keyed by `TextId` so symbol spelling storage is reused
- keep `Token` carrying `text_id` / `file_id`
- mark `Token::lexeme` and `Token::file` as deprecated bridge fields

Completion check:
- token-layer and parser-layer id surfaces are structurally in place and the
  bridge/deprecation intent is explicit in headers

## Step 2: Wire Lexer Token Construction

Goal: make the lexer populate `text_id` and `file_id` centrally while
preserving existing observable token behavior.

Primary targets:
- [src/frontend/lexer/lexer.hpp](/workspaces/c4c/src/frontend/lexer/lexer.hpp)
- [src/frontend/lexer/lexer.cpp](/workspaces/c4c/src/frontend/lexer/lexer.cpp)
- [src/apps/c4cll.cpp](/workspaces/c4c/src/apps/c4cll.cpp)

Actions:
- give `Lexer` owned `TextTable` / `FileTable` state or an equivalent shared
  frontend tables owner
- populate `text_id` for emitted tokens whose raw spelling must be preserved
- populate `file_id` for all emitted tokens
- keep `lexeme` / `file` filled during the bridge period so existing parser and
  `--lex-only` behavior stays stable
- decide whether any debug/inspection output should surface ids now or remain
  spelling-only for this slice

Completion check:
- all lexer-produced tokens carry stable `text_id` / `file_id` values without
  changing current lex-only output or keyword/literal classification

## Step 3: Add Parser-Owned Symbol Identity Surface

Goal: introduce parser-owned `SymbolId` access paths that sit on top of token
`TextId` values and are ready for semantic-table migration.

Primary targets:
- [src/frontend/parser/parser.hpp](/workspaces/c4c/src/frontend/parser/parser.hpp)
- [src/frontend/parser/parser_core.cpp](/workspaces/c4c/src/frontend/parser/parser_core.cpp)
- parser support files that host parser-owned semantic state helpers

Actions:
- attach parser-owned `TextTable` / `SymbolTable` state to the parser
- add helpers that derive `SymbolId` from identifier token `TextId` values or
  fallback spellings
- add parser-local name-table wrappers where that keeps string-to-id glue out
  of call sites
- keep the initial helper surface as small as possible and scoped to parser
  usage

Completion check:
- parser code can obtain stable `SymbolId` values for identifier atoms through
  one narrow helper path

## Step 4: Convert Parser Name Tables To Id Keys

Goal: migrate the parser's obvious source-atom semantic tables off raw string
keys.

Primary targets:
- [src/frontend/parser/parser.hpp](/workspaces/c4c/src/frontend/parser/parser.hpp)
- [src/frontend/parser/parser_core.cpp](/workspaces/c4c/src/frontend/parser/parser_core.cpp)
- parser files that read or write typedef/value-name state

Actions:
- convert typedef membership and typedef type lookup tables to `SymbolId` keys
- convert closely related variable/type disambiguation tables only where the
  lookup target is a plain identifier atom
- preserve current snapshot/restore behavior, changing key type only
- leave string-keyed tables in place where keys are composed or synthesized
  names

Completion check:
- typedef-name membership and typedef-type lookup on parser hot paths no longer
  depend on raw string keys for plain identifier atoms

## Step 5: Tighten Qualified-Name Boundaries

Goal: keep the new symbol model narrowly scoped so composed names do not leak
into the first slice.

Primary targets:
- parser-local qualified-name helpers and related semantic lookup sites

Actions:
- represent qualified-name segments as structured symbol-bearing parts where
  helpful
- leave composed canonical names and synthesized names string-based for now
- avoid broad refactors that conflate atom ids with full-name identity

Completion check:
- any migrated qualified-name handling still distinguishes segment identity from
  full-name string materialization

## Step 6: Validate And Trim Follow-On Work

Goal: confirm the migrated slice is stable and record anything intentionally
deferred.

Actions:
- run a targeted lexer/token validation slice that covers `text_id` / `file_id`
  population
- run targeted frontend/parser tests that exercise typedef disambiguation and
  parser-owned symbol lookup
- run rollback-sensitive parser coverage if parser snapshots were touched
- record any intentionally deferred string-keyed tables back into the source
  idea instead of silently widening this plan

Completion check:
- targeted validation passes and deferred work remains clearly out of scope
