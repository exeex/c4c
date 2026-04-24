Status: Active
Source Idea Path: ideas/open/94_parser_public_facade_pimpl_boundary.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Retarget Callers And Test Hooks

# Current Packet

## Just Finished

Completed Step 4 statement-parser test-hook isolation slice by removing
`Parser::parse_stmt` from public `src/frontend/parser/parser.hpp`, adding the
explicit `parse_stmt(Parser&)` private-boundary declaration to
`src/frontend/parser/impl/parser_impl.hpp`, converting the statement parser
definition and recursive implementation call sites to pass the parser
explicitly, and retargeting frontend parser tests to call the private helper.

## Suggested Next

Have the supervisor review the completed Step 4 parser facade-boundary slice
and decide whether the active runbook is ready for broader validation or plan
owner handling.

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
- `tests/frontend/frontend_parser_tests.cpp` now explicitly includes
  `impl/parser_impl.hpp` for private parser test hooks.
- Current source search found no remaining `Parser::parse_stmt` declaration or
  direct `parser.parse_stmt()` calls in the owned parser implementation/test
  files.

## Proof

Executor Step 4 focused proof passed for the statement parser test-hook
isolation slice:
`{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'; } > test_after.log 2>&1`

Result: passed. Proof log: `test_after.log`.
