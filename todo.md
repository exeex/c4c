Status: Active
Source Idea Path: ideas/open/94_parser_public_facade_pimpl_boundary.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Move Implementation-Only Method Declarations Behind The Boundary

# Current Packet

## Just Finished

Completed Step 3 declaration-boundary slice by removing the expression-only
`parse_sizeof_pack_expr`, `parse_new_expr`, and `parse_lambda_expr`
declarations from public `Parser` in `src/frontend/parser/parser.hpp`, adding
the equivalent private implementation helper declarations to
`src/frontend/parser/impl/parser_impl.hpp`, and converting the definitions and
call sites in `src/frontend/parser/parser_expressions.cpp` to pass `Parser&`
through the private implementation boundary.

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
- The delegated `rg` query confirms these helpers are no longer declared on
  public `Parser`; their declarations live in `impl/parser_impl.hpp`, with
  definitions and call sites in `parser_expressions.cpp` passing `Parser&`.

## Proof

Executor Step 3 focused proof passed for this declaration-boundary slice:
`{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'; } > test_after.log 2>&1`

Supervisor acceptance proof then escalated to a matching full-suite run:
`cmake --build build -j && ctest --test-dir build -j --output-on-failure`

Result: passed, 2974/2974 tests. Regression guard also passed against the
matching full-suite `test_before.log`.
