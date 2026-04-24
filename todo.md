Status: Active
Source Idea Path: ideas/open/94_parser_public_facade_pimpl_boundary.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Move Implementation-Only Method Declarations Behind The Boundary

# Current Packet

## Just Finished

Completed Step 3 declaration-boundary slice by removing the implementation-only
block parser entry from public `Parser` declarations in
`src/frontend/parser/parser.hpp`, adding the equivalent private
`parse_block(Parser& parser)` declaration to
`src/frontend/parser/impl/parser_impl.hpp`, converting the implementation in
`src/frontend/parser/parser_statements.cpp` to the private parser boundary,
and retargeting internal call sites in
`src/frontend/parser/parser_declarations.cpp`,
`src/frontend/parser/parser_expressions.cpp`, and
`src/frontend/parser/parser_types_struct.cpp`.

## Suggested Next

Continue with the next narrow parser facade boundary slice from the active
plan, keeping implementation-only declarations behind
`src/frontend/parser/impl/parser_impl.hpp` where practical.

## Watchouts

- Preserve parser behavior, AST output, diagnostics behavior, and testcase
  expectations.
- `parser.hpp` still includes `impl/parser_state.hpp` because current public and
  test-facing type aliases/signatures require complete snapshot, qualified-name,
  template-argument, and guard types; the object layout no longer stores those
  carrier objects directly.
- Keep implementation-only declarations behind
  `src/frontend/parser/impl/parser_impl.hpp` where practical.
- Keep any test-only hooks clearly named and isolated.
- `Parser` move operations are explicitly deleted in this slice because the
  compatibility boundary keeps public reference members bound into `ParserImpl`.
- `begin_record_body_context` remains publicly declared because
  `tests/frontend/frontend_parser_tests.cpp` calls it directly; the moved
  outer record-definition helpers had no external source or test references.
- `parse_enum` now lives behind `impl/parser_impl.hpp`; current source search
  found only parser implementation call sites.
- `parse_param` now lives behind `impl/parser_impl.hpp`; current source search
  found only parser implementation call sites.
- `parse_block` now lives behind `impl/parser_impl.hpp`; current source search
  found only parser implementation call sites, plus an audit-note mention under
  `src/frontend/parser/BOUNDARY_AUDIT.md`.

## Proof

Executor Step 3 focused proof passed for the block parser entry
declaration-boundary slice:
`{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'; } > test_after.log 2>&1`

Result: passed. Proof log: `test_after.log`.
