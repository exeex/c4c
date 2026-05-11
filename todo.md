Status: Active
Source Idea Path: ideas/open/168_compatibility_bridge_retirement.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Retire Parser Compatibility Overloads

# Current Packet

## Just Finished

Step 2 - Retire Parser Compatibility Overloads progressed for the selected
rendered typedef/type-compatibility helper family.

Completed work:

| Field | Result |
| --- | --- |
| Caller inspection | `rg` found production helper callers only in `src/frontend/hir/impl/templates/templates.cpp` and `src/frontend/hir/hir_functions.cpp`; both passed empty rendered typedef maps only to select the old overload. |
| Production caller updates | Moved those empty-map callers to empty `std::unordered_map<TextId, TypeSpec>` so they bind to the structured `TextId` overloads. |
| Parser support API | Deleted the rendered `std::unordered_map<std::string, TypeSpec>` overload declarations from `src/frontend/parser/parser_support.hpp` and definitions from `src/frontend/parser/impl/support.cpp`. No explicit no-metadata rendered boundary remains for this helper family because no non-test rendered caller remained. |
| Focused tests | Updated `tests/frontend/frontend_parser_lookup_authority_tests.cpp` to use the `TextId` typedef map for the parser-support nominal-identity compatibility checks. Existing stale-rendered-map regression tests in `frontend_parser_tests` and `cpp_hir_parser_type_helper_residual_metadata_test` stayed intact and still prove complete `TextId` misses do not recover through rendered maps. |
| Changed files | `src/frontend/parser/parser_support.hpp`, `src/frontend/parser/impl/support.cpp`, `src/frontend/hir/impl/templates/templates.cpp`, `src/frontend/hir/hir_functions.cpp`, `tests/frontend/frontend_parser_lookup_authority_tests.cpp`, `todo.md`, and `test_after.log`. |

## Suggested Next

Continue Step 2 with a separate parser-support bridge packet, likely rendered
record or constant-evaluation compatibility maps, after the supervisor selects
the next bridge family and proof subset.

## Watchouts

- The removed typedef/type-compatibility rendered overloads are gone from the
  parser-support public header; any future no-metadata rendered typedef use
  should be introduced only as an explicitly named compatibility boundary.
- `src/frontend/hir/hir_functions.cpp` still begins with a note saying the file
  is not wired into the build, but the delegated proof rebuilt it through
  `c4c_frontend`, so the direct production caller change was compiled.
- The pre-existing untracked `review/166_compile_time_registry_fencing_route_review.md`
  was not touched.
- No current blockers.

## Proof

Delegated proof command was run exactly and logged to `test_after.log`:

`cmake --build build --target frontend_parser_tests frontend_parser_lookup_authority_tests cpp_hir_parser_type_helper_residual_metadata_test cpp_hir_template_pattern_match_metadata_test && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_hir_parser_type_helper_residual_structured_metadata|cpp_hir_template_pattern_match_structured_metadata)$'`

Result: passed. Built the four delegated targets and ran 4/4 focused tests:
`frontend_parser_tests`, `frontend_parser_lookup_authority_tests`,
`cpp_hir_parser_type_helper_residual_structured_metadata`, and
`cpp_hir_template_pattern_match_structured_metadata`.

Additional hygiene: `git diff --check` passed.
