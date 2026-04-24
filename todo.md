Status: Active
Source Idea Path: ideas/open/94_parser_public_facade_pimpl_boundary.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Final Parser Facade Validation

# Current Packet

## Just Finished

Completed Step 7 final parser facade validation after the Step 5/6 boundary
transition removed `#include "impl/parser_state.hpp"` from
`src/frontend/parser/parser.hpp`. Acceptance checks found no public facade
include of `impl/parser_state.hpp`; private parser implementation files include
`impl/parser_impl.hpp`, and the only test include of `impl/parser_impl.hpp` is
the explicit private-hook parser test file.

## Suggested Next

Ask the plan owner to decide whether the active runbook and source idea are
close-ready, or whether any remaining facade cleanup should become a separate
follow-up idea.

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

Supervisor Step 7 final validation passed:
`ctest --test-dir build -j --output-on-failure`

Result: passed, 2974/2974 tests.
