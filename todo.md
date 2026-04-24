Status: Active
Source Idea Path: ideas/open/94_parser_public_facade_pimpl_boundary.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Separate Facade Types From Private State Carriers

# Current Packet

## Just Finished

Completed the next Step 5 facade boundary slice by moving the remaining
`parser.hpp` inline helper bodies that dereferenced private parser carriers into
`src/frontend/parser/parser_support.cpp`, which includes
`impl/parser_impl.hpp`. The moved helpers cover parser text lookup/interning,
current struct tag state, last resolved typedef state, last using-alias state,
function-pointer typedef lookup, parser symbol interning/spelling, and C++ mode
detection through `core_input_state_.source_profile`. `parser.hpp` now only
declares those methods; its only remaining inline method body is
`VisibleNameResult::operator bool()`, which does not touch private state.

Remaining `parser.hpp` dependencies on `impl/parser_state.hpp` are now
declaration/type-shape dependencies rather than inline private-state logic:
the `Parser` object still exposes reference members to private carriers
(`ParserCoreInputState`, `ParserSharedLookupState`, `ParserBindingState`,
`ParserDefinitionState`, `ParserTemplateState`, `ParserLexicalScopeState`,
`ParserActiveContextState`, `ParserNamespaceState`, `ParserDiagnosticState`,
and `ParserPragmaState`), and the public class still aliases/declares
state-backed snapshot and guard types (`ParserLiteSnapshot`, `ParserSnapshot`,
`TentativeParseStats`, `TentativeParseGuard*`, parse-context guard,
local-var-binding suppression guard, and template prelude guards). Those names
are what still force the private include unless the next packet replaces the
include with narrow forward declarations or moves those state-backed surfaces
behind implementation-only APIs.

## Suggested Next

Next executor packet: test whether `parser.hpp` can replace
`impl/parser_state.hpp` with narrow forward declarations for the remaining
private carrier, snapshot, and guard names, or identify the smallest public
surface that must move before that include can be removed.

## Watchouts

- Preserve parser behavior, AST output, diagnostics behavior, and testcase
  expectations.
- `parser.hpp` still includes `impl/parser_state.hpp`; this packet intentionally
  did not force final include removal.
- A source search now finds no `parser.hpp` inline methods that read/write
  `shared_lookup_state_`, `active_context_state_`, `binding_state_`,
  `core_input_state_`, or the other private state carrier members.
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

Executor Step 5 focused proof passed for the inline-helper move:
`{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'; } > test_after.log 2>&1`

Result: passed. Proof log: `test_after.log`.
