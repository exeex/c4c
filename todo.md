Status: Active
Source Idea Path: ideas/open/168_compatibility_bridge_retirement.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Retire Parser Compatibility Overloads

# Current Packet

## Just Finished

Step 2 - Retire Parser Compatibility Overloads progressed for the selected
`eval_const_int` rendered named-constant compatibility helper family.

Completed work:

| Field | Result |
| --- | --- |
| Caller inspection | `rg` found production `eval_const_int` callers in parser table wrappers, parser template static-member probes, and `src/frontend/hir/impl/templates/templates.cpp`; only the HIR static-member helper still had rendered named-constant input. |
| Production caller updates | Moved the empty rendered named-constant map in `eval_struct_static_member_value_hir` to an empty `std::unordered_map<TextId, long long>` so it binds to the structured overload. The real rendered NTTP path now calls an explicitly named compatibility function. Parser template callers already pass `ParserConstIntBindingTable` / `std::unordered_map<TextId, long long>`. |
| Parser support API | Removed the broad rendered `eval_const_int(..., std::unordered_map<std::string, long long>*)` overload from ordinary overload resolution. Added `eval_const_int_with_rendered_named_const_compatibility` for the remaining HIR/template no-metadata rendered boundary. |
| Focused tests | Added parser-support residual coverage proving structured named-constant evaluation uses `TextId`, fails closed after a complete `TextId` miss, and preserves no-metadata rendered behavior only through the explicit compatibility function. Existing stale-rendered-map regression tests were not weakened. |
| Changed files | `src/frontend/parser/parser_support.hpp`, `src/frontend/parser/impl/support.cpp`, `src/frontend/hir/impl/templates/templates.cpp`, `tests/frontend/cpp_hir_parser_support_residual_metadata_test.cpp`, `todo.md`, and `test_after.log`. |

## Suggested Next

Continue Step 2 with a separate parser-support bridge packet after the
supervisor selects the next bridge family and proof subset. The remaining
record/tag compatibility bridge is still intentionally out of this packet.

## Watchouts

- `eval_const_int_with_rendered_named_const_compatibility` is now the only
  parser-support rendered named-constant entry point; it exists for the HIR
  static-member rendered NTTP/no-metadata compatibility path and should not gain
  new parser-owned callers.
- Record-layout compatibility still flows through the existing rendered
  record/tag map where required; `resolve_record_type_spec` was not folded into
  this packet.
- The pre-existing untracked `review/166_compile_time_registry_fencing_route_review.md`
  was not touched.
- No current blockers.

## Proof

Delegated proof command was run exactly and logged to `test_after.log`:

`cmake --build build --target frontend_parser_tests frontend_parser_lookup_authority_tests cpp_hir_parser_support_residual_metadata_test cpp_hir_template_pattern_match_metadata_test && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_hir_parser_support_residual_structured_metadata|cpp_hir_template_pattern_match_structured_metadata)$'`

Result: passed. Built the four delegated targets and ran 4/4 focused tests:
`frontend_parser_tests`, `frontend_parser_lookup_authority_tests`,
`cpp_hir_parser_support_residual_structured_metadata`, and
`cpp_hir_template_pattern_match_structured_metadata`.

Additional hygiene: `git diff --check` passed.
