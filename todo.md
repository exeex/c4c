# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 narrowed parser AST template-dependency helpers in
`src/frontend/parser/ast.hpp`. `typespec_mentions_template_param` and
`template_arg_list_mentions_template_param` now treat `TypeSpec::tag_text_id`,
`TypeSpec::template_param_text_id`,
`TypeSpec::deferred_member_type_text_id`, and
`TemplateArgRef::nttp_text_id` as authoritative before rendered `tag`,
`deferred_member_type_name`, or `debug_text` fallback. Rendered fallback remains
available only when the relevant dependency helper sees no structured carrier.

Focused coverage in
`tests/frontend/frontend_parser_lookup_authority_tests.cpp` now proves stale
rendered dependency text cannot recover when non-matching structured carriers
exist for top-level type args, deferred member type names, and nested NTTP
template args, while no-carrier compatibility still works.

## Suggested Next

Continue Step 4 review after the next review trigger. The remaining rendered
dependency routes to scrutinize should be no-carrier fallbacks only; verify any
next route has no parsed expression, token, TextId, record-def, origin-key, or
other structured carrier before preserving rendered-string compatibility.

## Watchouts

- This packet intentionally did not edit HIR, LIR, BIR, backend, `plan.md`, or
  `ideas/open`.
- `typespec_has_template_param_dependency_carrier` is intentionally scoped to
  dependency carriers used by these AST helpers; it does not claim broader type
  identity authority for HIR/backend consumers.
- Rendered `tag`, `deferred_member_type_name`, and `debug_text` fallback still
  exists for no-carrier compatibility. Do not reclassify those fallback paths as
  semantic authority in later packets.
- The no-carrier compatibility routes remain separate Step 4 watch items. Do
  not count this packet alone as source-idea closure.
- Existing untracked `review/step4_*.md` artifacts were left untouched.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_.*|eastl_cpp_external_utility_frontend_basic_cpp)$' | tee test_after.log`.

Result: build completed successfully; CTest passed `886/886` matched tests.
Final proof log path: `test_after.log`.
