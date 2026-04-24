Status: Active
Source Idea Path: ideas/open/94_parser_public_facade_pimpl_boundary.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Move Implementation-Only Method Declarations Behind The Boundary

# Current Packet

## Just Finished

Completed Step 3 declaration-boundary slice by removing the implementation-only
record base-clause helpers `skip_record_base_specifier_tail`,
`try_parse_record_base_specifier`, and `parse_record_base_clause` from public
`Parser` declarations in `src/frontend/parser/parser.hpp`, adding equivalent
private helper declarations to `src/frontend/parser/impl/parser_impl.hpp`, and
converting definitions and call sites in
`src/frontend/parser/parser_types_struct.cpp` to pass `Parser&` through the
private boundary.

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
- The delegated `rg` query confirms the three record base-clause helpers are no
  longer declared on public `Parser`; their declarations live in
  `impl/parser_impl.hpp`, with definitions and call sites passing `Parser&`.

## Proof

Executor Step 3 focused proof passed for the record base-clause helper
declaration-boundary slice:
`{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'; } > test_after.log 2>&1`

Result: passed. Proof log: `test_after.log`.
