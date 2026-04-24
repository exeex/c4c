Status: Active
Source Idea Path: ideas/open/92_parser_agent_index_header_hierarchy.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Move Type Parsing Helpers Under `impl/types/`

# Current Packet

## Just Finished

- Completed plan Step 4 by moving the parser type-helper implementation body
  from `src/frontend/parser/types_helpers.hpp` to
  `src/frontend/parser/impl/types/types_helpers.hpp`.
- Retargeted frontend parser implementation include sites in
  `parser_declarations.cpp`, `parser_expressions.cpp`,
  `parser_statements.cpp`, `parser_types_base.cpp`,
  `parser_types_declarator.cpp`, `parser_types_struct.cpp`, and
  `parser_types_template.cpp` to include the private helper path directly.
- Kept `src/frontend/parser/types_helpers.hpp` as a thin compatibility bridge
  that includes `impl/types/types_helpers.hpp`; current parser implementation
  users no longer include the top-level bridge directly.
- Did not add `src/frontend/parser/impl/types/types.hpp`; the moved helper did
  not need a private type-helper index for this packet.

## Suggested Next

- Next coherent movement boundary: Step 5 should remove or retire now-unused
  top-level compatibility bridges after confirming no public or external
  include sites still depend on `parser_state.hpp` or `types_helpers.hpp`.

## Watchouts

- Both top-level bridges, `parser_state.hpp` and `types_helpers.hpp`, appear
  unused by current frontend parser implementation include sites after this
  packet, but they were intentionally preserved for incremental compatibility.
- The moved type helper is still an implementation-only include file with
  anonymous-namespace/static helpers copied per translation unit; the move did
  not change that linkage model.
- Backend assembler local `parser.hpp` files were not touched.

## Proof

- Ran exactly:
  `{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'; } > test_after.log 2>&1`
- Result: passed.
- Test subset: `frontend_parser_tests`.
- Proof log: `test_after.log`.
