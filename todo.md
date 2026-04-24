Status: Active
Source Idea Path: ideas/open/92_parser_agent_index_header_hierarchy.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Minimize Public Exposure Of Parser State

# Current Packet

## Just Finished

- Completed plan Step 3 sub-slice replacing direct `parser.tokens_`,
  `parser.pos_`, and `parser.definition_state_` access in
  `src/frontend/parser/impl/types/types_helpers.hpp`.
- Added narrow Parser helpers for token cursor probing, injected base-type
  parsing, defined-struct-tag lookup, and constant-expression evaluation through
  parser-owned tables.

## Suggested Next

- Next Step 3 sub-slice: continue removing parser helper/header dependencies on
  public Parser state, prioritizing direct `binding_state_`,
  `template_state_`, or `namespace_state_` reads that still force
  `impl/parser_state.hpp` through `parser.hpp`.

## Watchouts

- `types_helpers.hpp` no longer directly references `parser.tokens_`,
  `parser.pos_`, or `parser.definition_state_`; the delegated grep is clean.
- Remaining blockers to removing `impl/parser_state.hpp` from `parser.hpp`
  include public state members still read by parser implementation files and
  any helper headers that need state-shaped types from `impl/parser_state.hpp`.
- The new helpers are not test-only APIs; avoid routing implementation callers
  through existing `*_for_testing` helpers when continuing the state split.

## Proof

- Ran the supervisor-selected proof:
  `{ cmake --build build -j --target frontend_parser_tests c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'; } > test_after.log 2>&1`
- Result: passed. `test_after.log` is the proof log.
