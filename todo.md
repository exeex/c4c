Status: Active
Source Idea Path: ideas/open/92_parser_agent_index_header_hierarchy.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Minimize Public Exposure Of Parser State

# Current Packet

## Just Finished

- Completed plan Step 2 include-boundary split for parser/HIR support helpers.
- Added `src/frontend/parser/parser_support.hpp` as the narrow public support
  header for HIR-used free helpers: `sizeof_base(...)`,
  `sizeof_type_spec(...)`, `alignof_type_spec(...)`, `eval_const_int(...)`,
  `resolve_typedef_chain(...)`, and `types_compatible_p(...)`.
- Removed parser-support free-function declarations from `parser.hpp`, leaving
  the public parser header focused on the `Parser` facade and its still-public
  implementation state.
- Moved parser-only support declarations behind `impl/parser_impl.hpp`:
  `eval_enum_expr(...)`, `is_dependent_enum_expr(...)`,
  `effective_scalar_base(...)`, `align_base(...)`, `is_qualifier(...)`,
  `is_storage_class(...)`, `is_type_kw(...)`, `lexeme_is_imaginary(...)`,
  `parse_int_lexeme(...)`, and `parse_float_lexeme(...)`.
- Retargeted HIR support users in `hir_build.cpp`, `hir_functions.cpp`,
  `hir_types.cpp`, and `hir_lowerer_internal.hpp` to include
  `../parser/parser_support.hpp` instead of `../parser/parser.hpp`.
- Changed files: `todo.md`, `src/frontend/parser/parser.hpp`,
  `src/frontend/parser/parser_support.hpp`,
  `src/frontend/parser/impl/parser_impl.hpp`,
  `src/frontend/hir/hir_build.cpp`,
  `src/frontend/hir/hir_functions.cpp`,
  `src/frontend/hir/hir_types.cpp`, and
  `src/frontend/hir/hir_lowerer_internal.hpp`.

## Suggested Next

- Add a public parser diagnostic accessor/result route so production callers
  such as `src/apps/c4cll.cpp` no longer read `diagnostic_state_` directly.
  That is the next blocker before `ParserDiagnosticState` can move behind a
  private state handle.

## Watchouts

- Remaining facade/PIMPL blocker: `parser.hpp` still includes
  `impl/parser_state.hpp` because the public `Parser` object owns state fields
  directly, and app/test code still reaches some of those fields. This packet
  only split support free-function declarations; it did not change parser state
  layout or semantics.
- HIR no longer needs broad `parser.hpp` access for support helpers. Future HIR
  include additions should prefer `parser_support.hpp` unless they truly create
  a `Parser` object.

## Proof

- Ran exactly:
  `{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'; } > test_after.log 2>&1`
- Result: passed.
- Test subset used: `frontend_parser_tests`.
- Proof log path: `test_after.log`.
- Full-suite baseline candidate for commit `7298738b` was accepted.
