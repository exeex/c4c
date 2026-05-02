# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 removed one rendered `tag` semantic route outside the four audited
`base.cpp` helpers. In `try_parse_qualified_base_type`, the dependent
qualified-template probe no longer calls `parser.find_parser_text_id(arg.type.tag)`
before checking structured carriers; it now uses
`TemplateArgParseResult::type.template_param_text_id` first, then
`TypeSpec::tag_text_id`, and only falls back to rendered `tag` lookup for
legacy no-carrier template arguments.

## Suggested Next

Continue Step 4 review after the next review trigger. A likely next narrow
inspection route is the remaining Sema `validate.cpp` rendered-tag
compatibility mirrors (`complete_structs_`, `complete_unions_`, and
`structured_record_keys_by_tag_`) versus the existing `record_def` and
`namespace_context_id + tag_text_id` carriers.

## Watchouts

- This packet intentionally did not edit HIR, LIR, BIR, backend, `plan.md`, or
  `ideas/open`.
- The fallback left in the changed loop is intentionally no-carrier
  compatibility for parsed type args that lack both `template_param_text_id`
  and `tag_text_id`; do not treat it as semantic authority when either carrier
  is present.
- Remaining `deferred_member_type_name` and `debug_text` uses inspected in this
  packet were in parser display/canonicalization or existing no-carrier
  compatibility surfaces; no additional structured carrier was proven at those
  call sites within this slice.
- Existing untracked `review/step4_*.md` artifacts were left untouched.

## Proof

Ran the delegated proof command after the narrow code change:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_.*|eastl_cpp_external_utility_frontend_basic_cpp)$' | tee test_after.log`.

Result: build completed successfully; CTest passed `886/886` matched tests.
Final proof log path: `test_after.log`.
