# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.4.4.6
Current Step Title: Delete Or Park The Remaining Member-Typedef Mirror

## Just Finished

Step 2.4.4.5C added a structured dependent/template record member-typedef
carrier keyed by the direct record/member `QualifiedNameKey` plus member
`TextId`. `register_record_member_typedef_bindings()` now records
template/dependent member typedefs in
`dependent_record_member_typedefs_by_key` while leaving the existing
`source_tag::member` publication intact for the next deletion retry. Qualified
type lookup now consults the structured carrier before rendered fallback
storage, and `frontend_parser_tests` covers the carrier against stale rendered
`owner::member` storage.

## Suggested Next

Retry Step 2.4.4.6 by deleting only the legacy
`register_typedef_binding(parser_text_id_for_token(..., source_tag::member))`
publication in `register_record_member_typedef_bindings()`. Keep the new
dependent record/member carrier and re-run the same parser/Sema timeout probe
subset to confirm the three formerly timed-out positive fixtures still parse.

## Watchouts

- The carrier intentionally does not reconstruct or split rendered
  `owner::member` strings; it uses the existing record/member structured key
  produced at record finalization.
- The old rendered publication is still present in this packet, so the next
  deletion retry is the real proof that no reader still depends on that mirror.
- The supervisor-selected CTest regex matched five tests in this checkout; the
  named template-member parse tests in the regex did not appear as separate
  CTest entries in the generated test list.

## Proof

`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_positive_sema_eastl_slice7_piecewise_ctor_parse_cpp|cpp_positive_sema_step3_timeout_probe_baseline_parse_cpp|cpp_positive_sema_tuple_element_alias_mix_parse_cpp|cpp_positive_sema_qualified_member_typedef_functional_cast_frontend_cpp|cpp_positive_parser_template_member_typedef_cache_roundtrip_parse_cpp|cpp_positive_parser_template_member_type_direct_parse_cpp|cpp_positive_parser_template_member_type_inherited_parse_cpp|cpp_positive_parser_template_specialization_member_typedef_trait_parse_cpp)$' --output-on-failure) > test_after.log 2>&1`

Passed: build succeeded and CTest reported 5/5 matching tests passed.
Supervisor acceptance: `ctest --test-dir build -R '^cpp_' --output-on-failure`
reported 1105/1105 matching tests passed after a fresh build; canonical
`test_after.log` now contains this broader C++ run.
Canonical proof log: `test_after.log`.
