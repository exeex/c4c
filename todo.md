Status: Active
Source Idea Path: ideas/open/94_parser_public_facade_pimpl_boundary.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Separate Facade Types From Private State Carriers

# Current Packet

## Just Finished

Completed the first Step 5 facade-type split by adding
`src/frontend/parser/parser_types.hpp` and moving facade-facing parser type
definitions out of `src/frontend/parser/impl/parser_state.hpp`: symbol table
types, token mutation, function-pointer typedef info, namespace/qualified-name
refs, template argument/scope records, record body/recovery records, parse
debug/failure records, tentative-parse mode/text-ref records, alias-template
info, and enum/const-int binding table aliases. `parser.hpp` now includes this
public type layer before the private state header; `parser_state.hpp` includes
the same layer and retains the private state carriers and guard/snapshot
definitions.

Remaining `parser.hpp` dependencies on `impl/parser_state.hpp` are classified:
public facade API now separated into `parser_types.hpp`; explicit parser-test
hooks still include state-adjacent signatures such as
`parser_symbol_tables()`, token cursor replacement/inspection hooks,
`register_*_for_testing()`, and alias-template test registration; parser-private
state exposure still forcing the private include includes the `Parser` reference
members to `ParserCoreInputState`, `ParserSharedLookupState`,
`ParserBindingState`, `ParserDefinitionState`, `ParserTemplateState`,
`ParserLexicalScopeState`, `ParserActiveContextState`, `ParserNamespaceState`,
`ParserDiagnosticState`, and `ParserPragmaState`, plus inline methods in
`parser.hpp` that read or mutate those carriers. Guard/snapshot aliases
(`ParserLiteSnapshot`, `ParserSnapshot`, `TentativeParseStats`,
`TentativeParseGuard*`, and parse-context/local-var/template prelude guards)
remain state-backed and are bounded Step 6 candidates.

## Suggested Next

Next executor packet: continue Step 5/Step 6 boundary tightening by moving
inline `parser.hpp` helpers that dereference private state carriers behind
`impl/parser_impl.hpp` or out-of-line definitions, then reassess whether
`parser.hpp` can drop `impl/parser_state.hpp` without changing parser behavior.

## Watchouts

- Preserve parser behavior, AST output, diagnostics behavior, and testcase
  expectations.
- `parser.hpp` still includes `impl/parser_state.hpp`; this packet intentionally
  did not force final include removal.
- The new `parser_types.hpp` still includes AST/token definitions because the
  separated facade-facing records contain `TypeSpec`, `Node*`, `Token`, and
  `TokenKind`.
- `parser_state.hpp` now starts at private carriers (`ParserCoreInputState` and
  below) after importing the public/facade type layer.
- Keep implementation-only declarations behind
  `src/frontend/parser/impl/parser_impl.hpp` where practical.
- Do not reintroduce macro compatibility shims for parser member access.
- Keep any test-only hooks clearly named and isolated.
- `tests/frontend/frontend_parser_tests.cpp` now explicitly includes
  `impl/parser_impl.hpp` for private parser test hooks.
- Current source search found no remaining `Parser::parse_local_decl` or
  `Parser::parse_top_level` declarations, and no direct
  `parser.parse_local_decl()` or `parser.parse_top_level()` calls in the owned
  parser implementation/test files.

## Proof

Executor Step 5 focused proof passed for the facade-type split:
`{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'; } > test_after.log 2>&1`

Result: passed. Proof log: `test_after.log`.
