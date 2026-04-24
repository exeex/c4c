Status: Active
Source Idea Path: ideas/open/94_parser_public_facade_pimpl_boundary.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Move Implementation-Only Method Declarations Behind The Boundary

# Current Packet

## Just Finished

Completed Step 3 declaration-boundary slice by removing the implementation-only
`consume_adjacent_string_literal` declaration from public `Parser` in
`src/frontend/parser/parser.hpp`, adding the equivalent private helper
declaration to `src/frontend/parser/impl/parser_impl.hpp`, converting the
definition in `src/frontend/parser/parser_core.cpp` to take `Parser&`, and
updating implementation call sites in `parser_expressions.cpp` and
`parser_statements.cpp` to pass `*this` through the private boundary.

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
- The delegated `rg` query confirms `consume_adjacent_string_literal` is no
  longer declared on public `Parser`; its declaration lives in
  `impl/parser_impl.hpp`, with the definition and call sites passing `Parser&`.

## Proof

Executor Step 3 focused proof passed for the
`consume_adjacent_string_literal` declaration-boundary slice:
`{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'; } > test_after.log 2>&1`

Supervisor acceptance proof then escalated to a matching full-suite run:
`cmake --build build -j && ctest --test-dir build -j --output-on-failure`

Result: passed, 2974/2974 tests. Regression guard also passed against the
matching full-suite `test_before.log`.
