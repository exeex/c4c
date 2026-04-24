Status: Active
Source Idea Path: ideas/open/92_parser_agent_index_header_hierarchy.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Move Parser State Support Under `impl/`

# Current Packet

## Just Finished

- Completed plan Step 3 by moving parser-private state declarations from
  `src/frontend/parser/parser_state.hpp` to
  `src/frontend/parser/impl/parser_state.hpp`.
- Updated the moved private header's relative includes for `ast.hpp`,
  `token.hpp`, `source_profile.hpp`, and shared name-table headers.
- Retargeted `parser.hpp` to include `impl/parser_state.hpp` directly and
  updated `impl/parser_impl.hpp` to include the private state header as part of
  the private parser implementation index.
- Kept `src/frontend/parser/parser_state.hpp` as a thin compatibility bridge
  that includes `impl/parser_state.hpp`; no external direct include currently
  requires it, but it preserves incremental compatibility for Step 5 cleanup.

## Suggested Next

- Next coherent movement boundary: Step 4 should move `types_helpers.hpp`
  under `src/frontend/parser/impl/types/`, retarget its parser implementation
  include sites, and add a private type-helper index only if the moved helper
  needs one.

## Watchouts

- `parser.hpp` still exposes parser-private state transitively because the
  `Parser` class owns those state fields publicly; this packet only moved the
  state header behind the private path boundary without changing declarations.
- The top-level `parser_state.hpp` bridge has no direct include users after
  this packet and should be considered for removal in Step 5 after remaining
  private header moves settle.
- Keep Step 4 structural and behavior-preserving. Do not retarget backend
  assembler local `parser.hpp` files.
- `types_helpers.hpp` depends on parser implementation internals and is
  included from `parser_declarations.cpp`, `parser_expressions.cpp`, and
  `parser_statements.cpp` as well as type parser files, so the later
  `impl/types/` move is broader than the file comment's type-only list.

## Proof

- Ran exactly:
  `{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'; } > test_after.log 2>&1`
- Result: passed.
- Test subset: `frontend_parser_tests`.
- Proof log: `test_after.log`.
