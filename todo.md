Status: Active
Source Idea Path: ideas/open/92_parser_agent_index_header_hierarchy.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Minimize Public Exposure Of Parser State

# Current Packet

## Just Finished

- Completed plan Step 3 sub-slice to minimize public parser state exposure by
  adding narrow parser test/support helpers for fixture seeding of concept
  names, struct definitions, using-value aliases, and alias-template info.
- Converted fixture-only writes in
  `tests/frontend/frontend_parser_tests.cpp` away from direct
  `binding_state_`, `definition_state_`, `namespace_state_`, and
  `template_state_` access.
- Left the two direct `namespace_state_.using_value_aliases` parse-output
  inspections in place because they are white-box assertions/mutations rather
  than fixture seeding.

## Suggested Next

- Next Step 3 sub-slice: add narrow observation or mutation helpers for the
  remaining white-box parser-test state access families, starting with
  `active_context_state_` and `shared_lookup_state_`, while keeping the
  `namespace_state_.using_value_aliases` parse-output assertions separate from
  fixture seeding.

## Watchouts

- The `impl/types/types_helpers.hpp` direct `tokens_`/`pos_` usage is
  implementation-internal despite living in a header; do not classify it as a
  public caller. It is still a facade/PIMPL blocker because a future private
  state split must give parser helper headers a private cursor interface.
- Remaining direct parser-test access families found after this slice are
  `shared_lookup_state_`, `active_context_state_`, and two
  `namespace_state_.using_value_aliases` white-box checks. Avoid treating those
  checks as fixture seeding.
- Keep future helpers narrow; do not expose whole state carriers just to remove
  the remaining test accesses.

## Proof

- Ran the supervisor-selected proof:
  `{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'; } > test_after.log 2>&1`
- Result: passed. `test_after.log` is the proof log.
