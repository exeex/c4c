Status: Active
Source Idea Path: ideas/open/94_parser_public_facade_pimpl_boundary.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Move Implementation-Only Method Declarations Behind The Boundary

# Current Packet

## Just Finished

Completed Step 3 by removing the implementation-only `bin_prec` declaration
from public `Parser` in `src/frontend/parser/parser.hpp`, adding the equivalent
private implementation helper declaration to
`src/frontend/parser/impl/parser_impl.hpp`, and converting the definition in
`src/frontend/parser/parser_expressions.cpp` to an implementation free
function used by expression parsing.

## Suggested Next

Continue with the next parser facade boundary slice from the active plan,
keeping implementation-only declarations behind
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
- `rg` confirms `bin_prec` is no longer declared in public `parser.hpp`; the
  remaining declaration lives in `impl/parser_impl.hpp`, with the definition
  and call in `parser_expressions.cpp`.

## Proof

Executor Step 3 focused proof passed:
`{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'; } > test_after.log 2>&1`

Supervisor acceptance proof then escalated to a matching full-suite run:
`cmake --build build -j && ctest --test-dir build -j --output-on-failure`

Result: passed, 2974/2974 tests. Regression guard also passed against the
matching full-suite `test_before.log`.
