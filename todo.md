Status: Active
Source Idea Path: ideas/open/168_compatibility_bridge_retirement.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Retire Parser Compatibility Overloads

# Current Packet

## Just Finished

Step 2 - Retire Parser Compatibility Overloads progressed for the
`resolve_record_type_spec` parser record/tag compatibility bridge family.

Completed work:

| Field | Result |
| --- | --- |
| Caller inspection | `rg` found production rendered record/tag map callers in template static-member/base lookup paths: `src/frontend/parser/impl/types/template.cpp` and `src/frontend/parser/impl/types/base.cpp`. Ordinary production callers with no map were updated to the one-argument `resolve_record_type_spec`. |
| Parser support API | Removed the broad `resolve_record_type_spec(..., std::unordered_map<std::string, Node*>*)` ordinary boundary. Added `resolve_record_type_spec_with_parser_tag_map_compatibility` for the retained parser-local rendered tag mirror. |
| Layout behavior | Constant-layout helpers still require a complete direct `record_def` for structured records and only preserve TextId-less legacy rendered compatibility. Structured metadata misses continue to fail closed before any rendered-key fallback. |
| Production caller updates | Remaining parser static-member/base compatibility callers now opt into `resolve_record_type_spec_with_parser_tag_map_compatibility`; direct record-def callers use `resolve_record_type_spec(ts)`. |
| Focused tests | Strengthened parser-support residual coverage so ordinary record lookup proves it does not recover through parser rendered-tag maps, while explicit compatibility preserves the no-metadata fallback. Existing frontend parser stale-map closed-miss tests now call the explicit compatibility API. |
| Changed files | `src/frontend/parser/parser_support.hpp`, `src/frontend/parser/impl/support.cpp`, `src/frontend/parser/impl/parser_state.hpp`, `src/frontend/parser/impl/types/base.cpp`, `src/frontend/parser/impl/types/template.cpp`, `src/frontend/parser/impl/core.cpp`, `src/frontend/parser/impl/types/declarator.cpp`, `src/frontend/parser/impl/types/types_helpers.hpp`, `tests/frontend/cpp_hir_parser_support_residual_metadata_test.cpp`, `tests/frontend/frontend_parser_tests.cpp`, `todo.md`, and `test_after.log`. |

## Suggested Next

Continue Step 2 with the next supervisor-selected parser compatibility family,
or have the supervisor decide whether the parser bridge-retirement step is
complete after the typedef, const-int, type-compat, and record/tag packets.

## Watchouts

- `resolve_record_type_spec_with_parser_tag_map_compatibility` is now the only
  parser-support record resolver that accepts the rendered `struct_tag_def_map`.
  It should stay limited to parser-local compatibility paths.
- Constant-layout evaluation still takes a rendered tag map as a layout
  compatibility input, but structured records must carry direct `record_def`
  for layout; do not route structured layout misses into the explicit
  compatibility resolver.
- The pre-existing untracked `review/166_compile_time_registry_fencing_route_review.md`
  was not touched.
- No current blockers.

## Proof

Delegated proof command was run exactly and logged to `test_after.log`:

`cmake --build build --target frontend_parser_tests frontend_parser_lookup_authority_tests cpp_hir_parser_support_residual_metadata_test && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_hir_parser_support_residual_structured_metadata)$'`

Result: passed. Built the three delegated targets and ran 3/3 focused tests:
`frontend_parser_tests`, `frontend_parser_lookup_authority_tests`,
and `cpp_hir_parser_support_residual_structured_metadata`.

Additional hygiene: `git diff --check` passed.
