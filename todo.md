# Frontend Source-Atom Symbol Table And Id-Keyed Semantic Maps Todo

Status: Active
Source Idea: ideas/open/06_frontend_source_atom_symbol_table_and_id_keyed_semantic_maps.md
Source Plan: plan.md

## Current Slice

Active item: Warning-Driven Convergence B, trim parser-side deprecated-token
bridge usage on helper and injected-token paths that still sit adjacent to the
qualified-name migration surface.

Why this slice:
- the qualified-name boundary now carries per-segment atom ids while preserving
  string-keyed composed-name lookup, so the next useful warning-driven work is
  to narrow nearby parser helper paths rather than widen semantic-table scope
- compiler warnings still show substantial parser-owned `Token::lexeme` /
  `Token::file` bridge traffic, but most of it remains outside the current
  plan slice and should stay incremental
- the next useful edits should focus on parser helper or injected-token sites
  that directly support qualified-name parsing, not a repo-wide warning purge

## Completed

- [x] Added shared stable-id helper shape in
      [src/frontend/string_id_table.hpp](/workspaces/c4c/src/frontend/string_id_table.hpp)
- [x] Added token-layer `TextId` / `FileId`, `TextTable` / `FileTable`, and
      deprecated bridge fields in
      [src/frontend/lexer/token.hpp](/workspaces/c4c/src/frontend/lexer/token.hpp)
- [x] Added parser-side `SymbolId`, `SymbolTable`, and `ParserNameTables`
      skeleton in [src/frontend/parser/parser.hpp](/workspaces/c4c/src/frontend/parser/parser.hpp)
- [x] Added lexer-owned `TextTable` / `FileTable` state and centralized
      `make_token(...)` id population in
      [src/frontend/lexer/lexer.hpp](/workspaces/c4c/src/frontend/lexer/lexer.hpp)
      and [src/frontend/lexer/lexer.cpp](/workspaces/c4c/src/frontend/lexer/lexer.cpp)
- [x] Preserved empty legacy file strings while still assigning stable
      `FileId` values via
      [src/frontend/string_id_table.hpp](/workspaces/c4c/src/frontend/string_id_table.hpp)
- [x] Added narrow lexer-id coverage in
      [tests/frontend/frontend_lexer_tests.cpp](/workspaces/c4c/tests/frontend/frontend_lexer_tests.cpp)
      plus CMake / CTest wiring
- [x] Added parser-owned text/symbol helper state plus token-`TextId`-driven
      interning helpers in
      [src/frontend/parser/parser.hpp](/workspaces/c4c/src/frontend/parser/parser.hpp)
      and [src/frontend/parser/parser_core.cpp](/workspaces/c4c/src/frontend/parser/parser_core.cpp)
- [x] Added narrow parser symbol-helper coverage in
      [tests/frontend/frontend_parser_tests.cpp](/workspaces/c4c/tests/frontend/frontend_parser_tests.cpp)
      plus CMake / CTest wiring
- [x] Migrated parser typedef membership, typedef type lookup, user typedef
      membership, and var-type wrapper storage to `SymbolId` behind the
      existing string-facing parser helpers in
      [src/frontend/parser/parser.hpp](/workspaces/c4c/src/frontend/parser/parser.hpp),
      [src/frontend/parser/parser_core.cpp](/workspaces/c4c/src/frontend/parser/parser_core.cpp),
      and [src/frontend/parser/parser_support.cpp](/workspaces/c4c/src/frontend/parser/parser_support.cpp)
- [x] Added parser coverage for string-facing typedef / var-type wrappers over
      symbol-keyed storage plus heavy snapshot restore of symbol-keyed parser
      tables in
      [tests/frontend/frontend_parser_tests.cpp](/workspaces/c4c/tests/frontend/frontend_parser_tests.cpp)
- [x] Re-ran clean full-suite before/after regression logs and passed the
      monotonic guard with `3375` passed / `0` failed before and after via
      `test_fail_before.log`, `test_fail_after.log`, and the regression-guard
      checker
- [x] Added `QualifiedNameRef` atom-id carriage for real parse paths so parsed
      qualified names keep string spelling plus per-segment / base `SymbolId`
      values in [src/frontend/parser/parser.hpp](/workspaces/c4c/src/frontend/parser/parser.hpp),
      [src/frontend/parser/parser_core.cpp](/workspaces/c4c/src/frontend/parser/parser_core.cpp),
      and [src/frontend/parser/parser_types_declarator.cpp](/workspaces/c4c/src/frontend/parser/parser_types_declarator.cpp)
- [x] Added narrow parser coverage for parsed qualified-name atom ids in
      [tests/frontend/frontend_parser_tests.cpp](/workspaces/c4c/tests/frontend/frontend_parser_tests.cpp)
- [x] Re-ran clean full-suite before/after regression logs for the
      qualified-name atom-id slice and passed the monotonic guard with `3375`
      passed / `0` failed before and after via `test_fail_before.log`,
      `test_fail_after.log`, and the regression-guard checker

## Next Steps

- [x] Step 2A: give `Lexer` owned or shared `TextTable` / `FileTable` state and
      make `make_token(...)` populate `text_id` / `file_id`
- [x] Step 2B: keep `--lex-only` and existing parser callers behavior-stable
      while tokens start carrying ids
- [x] Step 3A: attach parser-owned symbol lookup helpers to token `TextId`
      rather than raw spelling first
- [x] Step 4A: migrate `typedefs_`, `user_typedefs_`, `typedef_types_`, and the
      atom-keyed subset of `var_types_` to `SymbolId`
- [x] Step 5A: review qualified-name helpers and keep full-name/composed-name
      paths string-keyed unless they are truly atom-segment based
      Current slice: add a narrow parser test for qualified/composed bindings,
      route non-atom typedef/var wrappers to string-keyed fallback storage, and
      keep that fallback rollback-safe under heavy snapshots
- [x] Step 5B: let real qualified-name parse paths carry per-segment atom ids
      while preserving current string materialization for composed lookups
- [x] Step 6A: validate the qualified-name boundary slice and record the
      remaining atom-segment follow-on work back into the source idea
- [ ] Warning-Driven Convergence B: audit parser helper / injected-token paths
      that still rely on deprecated token bridge fields near qualified-name
      parsing and trim the narrowest safe subset next

## Warning-Driven Convergence

- [ ] Keep `Token::lexeme` and `Token::file` under `[[deprecated]]`
- [ ] Use compiler warnings to reveal legacy call sites gradually
- [ ] Only remove or rewrite warninging call sites when they are directly on
      the current migration path

## Validation

- [x] Add or update one narrow validation path for lexer token ids before broad
      parser-table migration
- [x] Re-run the narrow lexer/parser slice after each connection point lands
- [x] Defer broad suite churn until Step 2 or Step 4 lands behaviorally useful
      changes
- [x] Re-ran the full suite for the qualified-name boundary slice and passed
      the monotonic guard with `3375` passed / `0` failed before and after via
      `test_fail_before.log`, `test_fail_after.log`, and the regression-guard
      checker
- [x] Re-ran the full suite for the qualified-name atom-id slice and passed
      the monotonic guard with `3375` passed / `0` failed before and after via
      `test_fail_before.log`, `test_fail_after.log`, and the regression-guard
      checker

## Resume Notes

- `SymbolId` remains parser-owned
- `Token` should not gain `symbol_id`
- `TextTable` owns raw string storage; `SymbolTable` reuses `TextId`
- `FileTable` now preserves the legacy empty file spelling under a stable
  `FileId`, so token ids are populated even for in-memory / no-line-marker
  lexer runs
- parser now keeps its own `TextTable` plus a token-`TextId` to parser-text-id
  cache so repeated identifier tokens reuse one parser-owned symbol path
- parser typedef and var-type wrapper storage now lives in symbol-keyed parser
  tables; string-facing helpers resolve through non-interning symbol lookup so
  unknown names do not create semantic entries
- prefixed string and char literals (`L`, `u`, `U`, `u8`) now intern the full
  prefixed raw spelling directly at token construction time
- qualified/composed typedef and var bindings now stay in string-keyed fallback
  maps, heavy snapshot save/restore includes that fallback state, and
  `QualifiedNameRef` owns the shared string spelling helper used by the parser
- parsed qualified-name paths now also populate per-segment / base atom ids in
  `QualifiedNameRef`, but lookahead-only probes still leave those ids empty and
  full-name lookup plus synthesized names remain string-keyed
- next edit should target the narrow parser helper / injected-token call sites
  that still rely on deprecated token bridge fields on the qualified-name
  boundary, without widening the migration into unrelated parser subsystems
