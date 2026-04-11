# Frontend Source-Atom Symbol Table And Id-Keyed Semantic Maps Todo

Status: Active
Source Idea: ideas/open/06_frontend_source_atom_symbol_table_and_id_keyed_semantic_maps.md
Source Plan: plan.md

## Current Slice

Active item: Step 2, wire lexer token construction so `Token::text_id` and
`Token::file_id` are populated centrally while `lexeme` / `file` stay as
deprecated bridge fields.

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

## Next Steps

- [ ] Step 2A: give `Lexer` owned or shared `TextTable` / `FileTable` state and
      make `make_token(...)` populate `text_id` / `file_id`
- [ ] Step 2B: keep `--lex-only` and existing parser callers behavior-stable
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

- [ ] Add or update one narrow validation path for lexer token ids before broad
      parser-table migration
- [ ] Re-run the narrow lexer/parser slice after each connection point lands
- [ ] Defer broad suite churn until Step 2 or Step 4 lands behaviorally useful
      changes

## Resume Notes

- `SymbolId` remains parser-owned
- `Token` should not gain `symbol_id`
- `TextTable` owns raw string storage; `SymbolTable` reuses `TextId`
- next edit should touch `src/frontend/lexer/lexer.hpp` and
  `src/frontend/lexer/lexer.cpp` before widening parser changes
