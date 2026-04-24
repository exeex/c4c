Status: Active
Source Idea Path: ideas/open/94_parser_public_facade_pimpl_boundary.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Tighten Public Header And Include Proof

# Current Packet

## Just Finished

Completed the Step 5/6 boundary transition by removing
`#include "impl/parser_state.hpp"` from `src/frontend/parser/parser.hpp`.
`parser.hpp` now forward-declares the private parser state carriers, snapshot
types, tentative/parse-context guards, and template prelude guard types it only
names by reference, alias, return type, or parameter type. The complete private
definitions now enter parser implementation translation units through
`src/frontend/parser/impl/parser_impl.hpp`, which explicitly includes
`parser_state.hpp` after the public parser facade. `parser.hpp` also now
includes `shared/qualified_name_table.hpp` directly because `QualifiedNameKey`
is a public facade signature dependency that was previously arriving
transitively through private state.

## Suggested Next

Next executor packet: start Step 6 by shrinking the remaining parser facade
exposure itself, especially public aliases and members that still name private
snapshot/guard/carrier types, now that the public header no longer includes
their definitions.

## Watchouts

- Preserve parser behavior, AST output, diagnostics behavior, and testcase
  expectations.
- `parser.hpp` no longer includes `impl/parser_state.hpp`; do not reintroduce
  private state definitions through another public include.
- `QualifiedNameKey` is an intentional public support dependency in
  `parser.hpp` because multiple facade declarations expose it.
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

Executor Step 5/6 boundary proof passed:
`{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'; } > test_after.log 2>&1`

Result: passed. Proof log: `test_after.log`.

Supervisor include proof passed:
`c++ -std=c++20 -I src -I src/frontend -I src/frontend/parser -I src/frontend/lexer -c /tmp/c4c_parser_header_probe.cpp -o /tmp/c4c_parser_header_probe.o`

Result: passed for a standalone source that includes only
`frontend/parser/parser.hpp`.
