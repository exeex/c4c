Status: Active
Source Idea Path: ideas/open/92_parser_agent_index_header_hierarchy.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce Private Parser Implementation Index

# Current Packet

## Just Finished

- Completed plan Step 2 by introducing
  `src/frontend/parser/impl/parser_impl.hpp` as the private parser
  implementation index.
- The new private index is a conservative compatibility bridge and includes
  `../parser.hpp`; no declarations were moved and parser semantics were not
  changed.
- Retargeted parser implementation translation units to include
  `impl/parser_impl.hpp` instead of `parser.hpp`:
  `parser_core.cpp`, `parser_declarations.cpp`, `parser_expressions.cpp`,
  `parser_statements.cpp`, `parser_support.cpp`, `parser_types_base.cpp`,
  `parser_types_declarator.cpp`, `parser_types_struct.cpp`, and
  `parser_types_template.cpp`.
- External callers remain on `parser.hpp`: `src/apps/c4cll.cpp`,
  `tests/frontend/frontend_parser_tests.cpp`, and
  `tests/frontend/frontend_hir_tests.cpp`.

## Suggested Next

- Next coherent movement boundary: Step 3 should relocate
  `parser_state.hpp` under `src/frontend/parser/impl/`, retarget the private
  include path through `impl/parser_impl.hpp` or parser-private includes, and
  keep a top-level compatibility bridge only if needed for external callers or
  parser tests during the incremental split.

## Watchouts

- This slice intentionally did not move private declarations out of
  `parser.hpp`; `impl/parser_impl.hpp` is only the private include route for
  implementation `.cpp` files so later declaration moves have a stable target.
- Keep Step 3 structural and behavior-preserving. Do not retarget backend
  assembler local `parser.hpp` files.
- `tests/frontend/frontend_parser_tests.cpp` still reaches parser internals
  through `parser.hpp`; moving `parser_state.hpp` may expose whether tests need
  temporary private-index access in a later packet.
- `src/apps/c4cll.cpp` still depends on public debug/error surfaces through
  `parser.hpp`; do not hide those until a public replacement or app retarget is
  deliberately planned.
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
