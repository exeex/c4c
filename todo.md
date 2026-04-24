Status: Active
Source Idea Path: ideas/open/92_parser_agent_index_header_hierarchy.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Closure Validation

# Current Packet

## Just Finished

- Reviewed plan Step 3 after the accessor/helper slices.
- Decided not to continue Step 3 as another narrow accessor pass: removing
  `impl/parser_state.hpp` from `parser.hpp` now requires changing `Parser`'s
  public object layout because the remaining state carrier fields are stored by
  value and parser implementation files are class methods over those fields.
- Created follow-on idea
  `ideas/open/94_parser_public_facade_pimpl_boundary.md` for the larger
  Parser facade/PIMPL rebuild needed to make external `parser.hpp` inclusion
  independent of private state definitions.

## Suggested Next

- Proceed with plan Step 4 closure validation for idea 92, treating full removal
  of `impl/parser_state.hpp` from `parser.hpp` as intentionally split into idea
  94 rather than as more Step 3 accessor work.
- Re-check that the completed header hierarchy work still satisfies the
  remaining idea-92 closure criteria once the facade/PIMPL constraint is
  excluded by the split.

## Watchouts

- `types_helpers.hpp` no longer directly references `parser.tokens_`,
  `parser.pos_`, or `parser.definition_state_`; the delegated grep is clean.
- Do not close idea 92 by claiming that `parser.hpp` no longer includes private
  parser state; it still includes `impl/parser_state.hpp`.
- The remaining exposure is recorded as a deliberate split because resolving it
  requires a public facade/private implementation ownership boundary, not just
  moving helper declarations or adding accessors.
- Do not start idea 94 inside the active plan unless the supervisor explicitly
  switches lifecycle state.

## Proof

- Ran the supervisor-selected proof:
  `{ cmake --build build -j --target frontend_parser_tests c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'; } > test_after.log 2>&1`
- Result: passed. `test_after.log` is the proof log.
- Lifecycle-only plan-owner review; no new build or test run.
