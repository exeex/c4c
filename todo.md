Status: Active
Source Idea Path: ideas/open/94_parser_public_facade_pimpl_boundary.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Retarget Callers And Test Hooks

# Current Packet

## Just Finished

Completed Step 4 test-hook isolation slice by removing the test-only
`Parser::parse_unary` expression hook from public `Parser` declarations in
`src/frontend/parser/parser.hpp`, adding the equivalent private-boundary
`parse_unary(Parser&)` declaration to
`src/frontend/parser/impl/parser_impl.hpp`, converting the implementation in
`src/frontend/parser/parser_expressions.cpp` to the private helper form, and
retargeting parser implementation and frontend parser test callers to pass the
parser explicitly.

## Suggested Next

Have the supervisor review the Step 4 facade-boundary slice and decide whether
the active parser public facade runbook is ready for broader validation or plan
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
- `consume_declarator_post_pointer_qualifiers` remains publicly declared
  because `tests/frontend/frontend_parser_tests.cpp` calls it directly.
- `parse_qualified_declarator_name` remains publicly declared because
  `tests/frontend/frontend_parser_tests.cpp` calls it directly.
- `tests/frontend/frontend_parser_tests.cpp` now explicitly includes
  `impl/parser_impl.hpp` for the private `parse_unary(Parser&)` test hook.
- Current source search found no remaining `Parser::parse_unary` declaration or
  direct member call in the owned parser implementation/test files.

## Proof

Executor Step 4 focused proof passed for the `parse_unary` test-hook isolation
slice:
`{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'; } > test_after.log 2>&1`

Result: passed. Proof log: `test_after.log`.
