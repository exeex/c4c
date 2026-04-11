# Frontend Source-Atom Symbol Table And Id-Keyed Semantic Maps Todo

Status: Active
Source Idea: ideas/open/06_frontend_source_atom_symbol_table_and_id_keyed_semantic_maps.md
Source Plan: plan.md

## Current Slice

Active item: Warning-Driven Convergence I, trim the next nearby read-only
template-argument type-head probes in `parser_types_declarator.cpp`, starting
with `is_simple_known_template_type_head()` and the forwarded-NTTP rejection
path that still read deprecated identifier spellings during speculative
parsing.

Why this slice:
- Warning-Driven Convergence H landed cleanly and moved the nearby
  `TemplateStruct<Args>::member` suffix probe onto `parser.token_spelling(...)`
- the next visible deprecated bridge reads on the current migration path sit in
  parser-local declarator template-argument lookahead, where the logic is still
  read-only and speculative
- those probes remain narrow enough to migrate without widening semantic-table
  scope or disturbing parser-state rollback behavior
- compiler warnings still show broader deprecated bridge traffic elsewhere, but
  those sites remain outside the current incremental path

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
- [x] Centralized parser token spelling and injected-token construction for the
      qualified-name boundary, switched `peek_qualified_name(...)` plus nearby
      dependent-typename injected reparse assembly onto those helpers, added
      narrow parser coverage for helper-built qualified-name tokens, and
      re-ran the full-suite monotonic guard with `3375` passed / `0` failed
      before and after via `test_before.log`, `test_after.log`, and the
      regression-guard checker
- [x] Moved `types_helpers.hpp` value-like template lookahead and
      template-arg expression text capture onto `parser.token_spelling(...)`,
      added narrow parser coverage for injected-token lookahead and
      expression-text capture in
      [tests/frontend/frontend_parser_tests.cpp](/workspaces/c4c/tests/frontend/frontend_parser_tests.cpp),
      and re-ran the full-suite monotonic guard with `3375` passed / `0`
      failed before and after via `test_before.log`, `test_after.log`, and the
      regression-guard checker
- [x] Moved the read-only identifier-classification and type-start probes in
      [src/frontend/parser/parser_types_base.cpp](/workspaces/c4c/src/frontend/parser/parser_types_base.cpp)
      onto `parser.token_spelling(...)`, added narrow parser coverage for
      injected typedef / concept probe spelling in
      [tests/frontend/frontend_parser_tests.cpp](/workspaces/c4c/tests/frontend/frontend_parser_tests.cpp),
      and re-ran the full-suite monotonic guard with `3375` passed / `0`
      failed before and after via `test_fail_before.log`,
      `test_fail_after.log`, and the regression-guard checker
- [x] Moved the nearby read-only exception-spec (`noexcept` / `throw`) and
      GNU attribute-name probes in
      [src/frontend/parser/parser_types_base.cpp](/workspaces/c4c/src/frontend/parser/parser_types_base.cpp)
      onto `parser.token_spelling(...)`, added narrow parser coverage for
      injected exception-spec and attribute spelling in
      [tests/frontend/frontend_parser_tests.cpp](/workspaces/c4c/tests/frontend/frontend_parser_tests.cpp),
      and re-ran the full-suite monotonic guard with `3375` passed / `0`
      failed before and after via `test_fail_before.log`,
      `test_fail_after.log`, and the regression-guard checker
- [x] Moved the nearby read-only builtin-transform / typeof-like spelling
      probes in
      [src/frontend/parser/parser_types_base.cpp](/workspaces/c4c/src/frontend/parser/parser_types_base.cpp)
      onto `parser.token_spelling(...)`, added narrow parser coverage for
      injected `__underlying_type`, `nullptr`, identifier operands, and float
      literal suffix inspection in
      [tests/frontend/frontend_parser_tests.cpp](/workspaces/c4c/tests/frontend/frontend_parser_tests.cpp),
      and re-ran the full-suite monotonic guard with `3375` passed / `0`
      failed before and after via `test_fail_before.log`,
      `test_fail_after.log`, and the regression-guard checker
- [x] Moved the nearby read-only `parse_base_type()` fixed-width-float,
      typedef/type-param visibility, and unresolved identifier-type fallback
      probes in
      [src/frontend/parser/parser_types_base.cpp](/workspaces/c4c/src/frontend/parser/parser_types_base.cpp)
      onto parser-owned token spelling helpers, added narrow parser coverage
      for injected `_Float128`, typedef, and unresolved identifier heads in
      [tests/frontend/frontend_parser_tests.cpp](/workspaces/c4c/tests/frontend/frontend_parser_tests.cpp),
      added a token-level fast path so real lexer tokens skip parser-owned
      spelling lookups unless they were parser-injected, and re-ran the
      full-suite monotonic guard with `3375` passed / `0` failed before and
      after via `test_fail_before.log`, `test_fail_after.log`, and the
      regression-guard checker
- [x] Moved the nearby read-only `TemplateStruct<Args>::member` suffix probe
      in
      [src/frontend/parser/parser_types_base.cpp](/workspaces/c4c/src/frontend/parser/parser_types_base.cpp)
      onto `parser.token_spelling(...)`, added narrow parser coverage for an
      injected `Trait<0>::type` suffix whose deprecated bridge lexeme is
      intentionally corrupted in
      [tests/frontend/frontend_parser_tests.cpp](/workspaces/c4c/tests/frontend/frontend_parser_tests.cpp),
      and re-ran the full-suite monotonic guard with `3375` passed / `0`
      failed before and after via `test_fail_before.log`,
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
- [x] Warning-Driven Convergence B: add parser token spelling / injected-token
      helpers, route qualified-name peeking plus nearby injected reparse paths
      through them, and keep the bridge fallback centralized
- [x] Warning-Driven Convergence C: move the next read-only parser helper
      lookahead sites in `types_helpers.hpp` off open-coded deprecated token
      spelling access without reintroducing parser-state mutation during peeks
- [x] Warning-Driven Convergence D: move the next read-only
      identifier-classification and type-start probe sites in
      `parser_types_base.cpp` onto parser-owned token spelling helpers without
      widening semantic-table scope
- [x] Warning-Driven Convergence E: move the next nearby read-only
      exception-spec and attribute-name spelling probes in
      `parser_types_base.cpp` onto parser-owned token helpers without widening
      semantic-table scope
- [x] Warning-Driven Convergence F: move the next nearby read-only
      builtin-transform / typeof-like spelling probes in
      `parser_types_base.cpp` onto parser-owned token helpers without widening
      semantic-table scope
- [x] Warning-Driven Convergence G: move the next nearby read-only
      identifier/type classification probes in `parser_types_base.cpp` onto
      parser-owned token helpers without widening semantic-table scope
- [x] Warning-Driven Convergence H: move the next nearby read-only
      template-member suffix probe in `parser_types_base.cpp` onto
      parser-owned token helpers without widening semantic-table scope
- [ ] Warning-Driven Convergence I: move the next nearby read-only
      template-argument type-head probes in `parser_types_declarator.cpp`
      onto parser-owned token helpers without widening semantic-table scope

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
- [x] Re-ran the full suite for Warning-Driven Convergence D and passed the
      monotonic guard with `3375` passed / `0` failed before and after via
      `test_fail_before.log`, `test_fail_after.log`, and the regression-guard
      checker
- [x] Re-ran the full suite for Warning-Driven Convergence E and passed the
      monotonic guard with `3375` passed / `0` failed before and after via
      `test_fail_before.log`, `test_fail_after.log`, and the regression-guard
      checker
- [x] Re-ran the full suite for Warning-Driven Convergence F and passed the
      monotonic guard with `3375` passed / `0` failed before and after via
      `test_fail_before.log`, `test_fail_after.log`, and the regression-guard
      checker
- [x] Re-ran the full suite for Warning-Driven Convergence G and passed the
      monotonic guard with `3375` passed / `0` failed before and after via
      `test_fail_before.log`, `test_fail_after.log`, and the regression-guard
      checker
- [x] Re-ran the full suite for Warning-Driven Convergence H and passed the
      monotonic guard with `3375` passed / `0` failed before and after via
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
- `types_helpers.hpp` now routes value-like template lookahead and template-arg
  text capture through `parser.token_spelling(...)`, with injected-token tests
  proving those helpers prefer parser-owned text ids over the deprecated bridge
- `parser_types_base.cpp` now uses `token_spelling(...)` for the
  `TemplateStruct<Args>::member` suffix probe, with injected `Trait<0>::type`
  coverage proving parser-owned spelling wins even when the deprecated member
  lexeme is corrupted
  identifier-classification / type-start probes plus the local
  `noexcept` / `throw` / GNU-attribute-name reads, the typeof-like probes, and
  the `parse_base_type()` fixed-width-float / typedef / unresolved identifier
  classification path
- `Token` now carries a parser-owned-spelling fast-path flag so injected tokens
  still prefer parser-owned text ids while ordinary lexer tokens avoid the
  extra parser spelling-map lookup on hot parse paths
- the next nearby warning candidate in `parser_types_base.cpp` is the local
  `TemplateStruct<Args>::member` suffix read around the deferred member-typedef
  path
