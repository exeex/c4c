Status: Active
Source Idea Path: ideas/open/94_parser_public_facade_pimpl_boundary.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Move Implementation-Only Method Declarations Behind The Boundary

# Current Packet

## Just Finished

Completed Step 3 declaration-boundary slice by removing the implementation-only
non-parenthesized declarator helper trio
`Parser::parse_non_parenthesized_declarator`,
`Parser::parse_non_parenthesized_declarator_tail`, and
`Parser::parse_plain_function_declarator_suffix` from public `Parser`
declarations in `src/frontend/parser/parser.hpp`, adding equivalent private
`Parser&` helper declarations to
`src/frontend/parser/impl/parser_impl.hpp`, converting the implementations in
`src/frontend/parser/parser_types_declarator.cpp` to the private parser
boundary, and retargeting internal parser call sites in
`src/frontend/parser/parser_types_declarator.cpp` and
`src/frontend/parser/parser_types_struct.cpp` to pass the parser explicitly.

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
- `parse_initializer` and `parse_init_list` now live behind
  `impl/parser_impl.hpp`; current source/test search found no remaining public
  declaration or external direct calls.
- `parse_static_assert_declaration` now lives behind `impl/parser_impl.hpp`;
  current source/test search found no remaining public declaration or external
  direct calls.
- `parse_top_level_parameter_list` now lives behind `impl/parser_impl.hpp`;
  current source/test search found no remaining public declaration, member
  definition, or external direct member calls.
- `parse_declarator_parameter_list` now lives behind `impl/parser_impl.hpp`;
  current source/test search found no remaining public declaration, member
  definition, or external direct member calls.
- `parse_parenthesized_function_pointer_suffix` now lives behind
  `impl/parser_impl.hpp`; current source/test search found no remaining public
  declaration, member definition, or external direct member calls.
- `parse_parenthesized_pointer_declarator_prefix`,
  `skip_parenthesized_pointer_declarator_array_chunks`, and
  `parse_parenthesized_pointer_declarator_name` now live behind
  `impl/parser_impl.hpp`; current source/test search found no remaining public
  declarations, member definitions, or external direct member calls.
- `try_parse_nested_parenthesized_pointer_declarator` and
  `parse_parenthesized_pointer_declarator_inner` now live behind
  `impl/parser_impl.hpp`; current source/test search found no remaining public
  declarations, member definitions, or external direct member calls.
- `parse_one_declarator_array_dim`, `parse_declarator_array_suffixes`, and
  `apply_declarator_array_dims` now live behind `impl/parser_impl.hpp`;
  current source/test search found no remaining public declarations, member
  definitions, or external direct member calls.
- `finalize_parenthesized_pointer_declarator` and
  `parse_parenthesized_pointer_declarator` now live behind
  `impl/parser_impl.hpp`; current source/test search found no remaining public
  declarations, member definitions, or external direct member calls.
- `parse_non_parenthesized_declarator`,
  `parse_non_parenthesized_declarator_tail`, and
  `parse_plain_function_declarator_suffix` now live behind
  `impl/parser_impl.hpp`; current source/test search found no remaining public
  declarations, member definitions, or external direct member calls.

## Proof

Executor Step 3 focused proof passed for the non-parenthesized declarator
helper declaration-boundary slice:
`{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'; } > test_after.log 2>&1`

Result: passed. Proof log: `test_after.log`.
