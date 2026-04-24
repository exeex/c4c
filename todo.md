Status: Active
Source Idea Path: ideas/open/94_parser_public_facade_pimpl_boundary.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce Opaque Parser Implementation Ownership

# Current Packet

## Just Finished

Completed Step 2 by adding `ParserImpl` ownership in
`src/frontend/parser/impl/parser_impl.hpp`, making `Parser` own that state
through `std::unique_ptr<ParserImpl>`, rebinding existing state-facing members
as references into the owned implementation object, and defining ownership
special members out of line in `parser_core.cpp`. Constructor and `parse()`
behavior were preserved.

## Suggested Next

Execute the next coherent parser facade packet: start moving implementation-only
helper declarations or test-only access away from the public facade now that the
state carrier has an owned implementation object.

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

## Proof

Executor Step 2 focused proof passed:
`{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'; } > test_after.log 2>&1`

Supervisor acceptance proof then escalated to a matching full-suite run:
`cmake --build build -j && ctest --test-dir build -j --output-on-failure`

Result: passed, 2974/2974 tests. Regression guard also passed against the
matching full-suite `test_before.log`.
