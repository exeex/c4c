# Frontend Source-Atom Symbol Table And Id-Keyed Semantic Maps Todo

Status: Active
Source Idea: ideas/open/06_frontend_source_atom_symbol_table_and_id_keyed_semantic_maps.md
Source Plan: plan.md

## Current Slice

Active item: Step 3A, attach parser-owned symbol lookup helpers to token
`TextId` values now that lexer-produced tokens carry stable `text_id` /
`file_id` values through the existing bridge fields.

Why this slice:
- current headers already define the storage shapes
- lexer population is the first real runtime connection point
- parser-side `SymbolId` wiring should sit on top of actual token `TextId`
  values rather than placeholder defaults

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

## Next Steps

- [x] Step 2A: give `Lexer` owned or shared `TextTable` / `FileTable` state and
      make `make_token(...)` populate `text_id` / `file_id`
- [x] Step 2B: keep `--lex-only` and existing parser callers behavior-stable
      while tokens start carrying ids
- [ ] Step 3A: attach parser-owned symbol lookup helpers to token `TextId`
      rather than raw spelling first
- [ ] Step 4A: migrate `typedefs_`, `user_typedefs_`, `typedef_types_`, and the
      atom-keyed subset of `var_types_` to `SymbolId`
- [ ] Step 5A: review qualified-name helpers and keep full-name/composed-name
      paths string-keyed unless they are truly atom-segment based

## Warning-Driven Convergence

- [ ] Keep `Token::lexeme` and `Token::file` under `[[deprecated]]`
- [ ] Use compiler warnings to reveal legacy call sites gradually
- [ ] Only remove or rewrite warninging call sites when they are directly on
      the current migration path

## Validation

- [x] Add or update one narrow validation path for lexer token ids before broad
      parser-table migration
- [ ] Re-run the narrow lexer/parser slice after each connection point lands
- [ ] Defer broad suite churn until Step 2 or Step 4 lands behaviorally useful
      changes

## Resume Notes

- `SymbolId` remains parser-owned
- `Token` should not gain `symbol_id`
- `TextTable` owns raw string storage; `SymbolTable` reuses `TextId`
- `FileTable` now preserves the legacy empty file spelling under a stable
  `FileId`, so token ids are populated even for in-memory / no-line-marker
  lexer runs
- prefixed string and char literals (`L`, `u`, `U`, `u8`) now intern the full
  prefixed raw spelling directly at token construction time
- next edit should stay inside parser symbol-lookup helpers and use token
  `TextId` values rather than widening lexer changes
