Status: Active
Source Idea Path: ideas/open/92_parser_agent_index_header_hierarchy.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Minimize Public Exposure Of Parser State

# Current Packet

## Just Finished

- Completed plan Step 3 sub-slice to minimize public parser state exposure by
  adding narrow parser test/support helpers for injected token-stream
  replacement, token inspection, and cursor observation.
- Converted `tests/frontend/frontend_parser_tests.cpp` away from direct
  `parser.tokens_`, `resolved_parser.tokens_`, and `parser.pos_` access.
- The replacement keeps parser behavior and testcase expectations unchanged;
  remaining direct parser state access in the frontend parser tests belongs to
  other state families.

## Suggested Next

- Next Step 3 sub-slice: add a similarly narrow parser test/support facade for
  the fixture-only state seeding still done through `binding_state_`,
  `definition_state_`, `namespace_state_`, and `template_state_`, then convert
  those direct test writes without changing expectations.

## Watchouts

- The `impl/types/types_helpers.hpp` direct `tokens_`/`pos_` usage is
  implementation-internal despite living in a header; do not classify it as a
  public caller. It is still a facade/PIMPL blocker because a future private
  state split must give parser helper headers a private cursor interface.
- `shared_lookup_state_` and `active_context_state_` reads in tests are still
  white-box assertions and should be handled separately from fixture seeding.
- Keep future helpers narrow; do not expose whole state carriers just to remove
  the remaining test accesses.

## Proof

- Ran the supervisor-selected proof:
  `{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'; } > test_after.log 2>&1`
- Result: passed. `test_after.log` is the proof log.
