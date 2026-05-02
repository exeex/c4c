# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 narrowed alias-template nested type-argument substitution in
`src/frontend/parser/impl/types/base.cpp`. `substitute_template_arg_ref_structured`
now treats `TypeSpec::tag_text_id` and `TypeSpec::template_param_text_id` as
structured template-parameter carriers even when rendered `TypeSpec::tag`
spelling is absent, so TextId-only nested type args no longer fall through to
stale `TemplateArgRef::debug_text`.

Focused coverage in
`tests/frontend/frontend_parser_lookup_authority_tests.cpp` now proves
`Alias<int>` substitutes a nested `Carrier<T>` argument from a TextId-only
carrier and rejects stale rendered debug text.

## Suggested Next

Continue Step 4 review after the next review trigger. A likely next narrow
inspection route is the sibling alias/member-typedef carrier substitution
helpers that still derive a parameter key from rendered spelling only when no
TextId carrier is present; verify each is true no-carrier compatibility before
preserving it.

## Watchouts

- This packet intentionally did not edit HIR, LIR, BIR, backend, `plan.md`, or
  `ideas/open`.
- The patched helper intentionally preserves rendered `tag` lookup only when
  both `tag_text_id` and `template_param_text_id` are absent.
- Rendered `tag`, `deferred_member_type_name`, and `debug_text` fallback still
  exists for no-carrier compatibility. Do not reclassify those fallback paths
  as semantic authority in later packets.
- The no-carrier compatibility routes remain separate Step 4 watch items. Do
  not count this packet alone as source-idea closure.
- Existing untracked `review/step4_*.md` artifacts were left untouched.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_.*|eastl_cpp_external_utility_frontend_basic_cpp)$' | tee test_after.log`.

Result: build completed successfully; CTest passed `886/886` matched tests.
Final proof log path: `test_after.log`.
