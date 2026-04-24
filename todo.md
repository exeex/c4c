Status: Active
Source Idea Path: ideas/open/94_parser_public_facade_pimpl_boundary.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Retarget Callers And Test Hooks

# Current Packet

## Just Finished

Completed Step 4 declaration and translation-unit parser hook isolation slice
by removing `Parser::parse_local_decl` and `Parser::parse_top_level` from public
`src/frontend/parser/parser.hpp`, adding explicit `parse_local_decl(Parser&)`
and `parse_top_level(Parser&)` private-boundary declarations to
`src/frontend/parser/impl/parser_impl.hpp`, retargeting `Parser::parse()`,
statement parser call sites, declaration-parser recursive calls, and frontend
parser test hooks to the private helpers, while preserving public
`Parser::parse()` behavior. Revised the declaration parser conversion to use
explicit `parser.` member access and `Parser::...` type names without a macro
compatibility shim.

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
- Do not reintroduce macro compatibility shims for parser member access.
- Keep any test-only hooks clearly named and isolated.
- `tests/frontend/frontend_parser_tests.cpp` now explicitly includes
  `impl/parser_impl.hpp` for private parser test hooks.
- Current source search found no remaining `Parser::parse_local_decl` or
  `Parser::parse_top_level` declarations, and no direct
  `parser.parse_local_decl()` or `parser.parse_top_level()` calls in the owned
  parser implementation/test files.

## Proof

Executor Step 4 focused proof passed for the declaration and translation-unit
parser hook isolation slice:
`{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'; } > test_after.log 2>&1`

Result: passed. Proof log: `test_after.log`.

Supervisor broader validation for the same slice also passed:
`ctest --test-dir build -j --output-on-failure`

Result: passed, 2974/2974 tests.
