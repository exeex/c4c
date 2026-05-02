# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 audited the remaining `base.cpp` fallback/helper surfaces called out by
review: `template_instantiation_name_key_for_direct_emit`,
`nttp_binding_metadata_for_template_param`,
`zero_value_arg_ref_uses_debug_fallback`, and
`unstructured_type_arg_ref_uses_debug_fallback`.

No code slice was taken. The direct-emission helpers already prefer parser
metadata from the template primary (`unqualified_text_id`,
`template_param_name_text_ids`, namespace context, and derived
`QualifiedNameKey`) and only use fallback names when primary metadata is absent.
The two debug helpers are classification guards for true no-expression or
unstructured `TemplateArgRef` compatibility; display reconstruction checks the
existing structured carriers before consulting them, and semantic instantiation
callers use them to reject debug-only refs rather than recover a record from
rendered spelling.

## Suggested Next

Continue Step 4 review after the next review trigger. A likely next narrow
inspection route is outside the four audited `base.cpp` helpers: recheck any
remaining rendered `tag`, `deferred_member_type_name`, or `debug_text` use only
where a semantic consumer still has a `TextId`, `QualifiedNameKey`,
`record_def`, origin key, or expression carrier available.

## Watchouts

- This packet intentionally did not edit HIR, LIR, BIR, backend, `plan.md`, or
  `ideas/open`.
- The audited direct-emission helpers still accept fallback spellings for
  absent primary metadata. Treat that as direct-emission/no-metadata
  compatibility, not as completed source-idea closure.
- `zero_value_arg_ref_uses_debug_fallback` and
  `unstructured_type_arg_ref_uses_debug_fallback` must stay classification
  guards. Do not use them as semantic recovery once a structured carrier is
  available.
- Rendered `tag`, `deferred_member_type_name`, and `debug_text` fallback still
  exists elsewhere for no-carrier compatibility. Do not reclassify those paths
  as semantic authority in later packets.
- Existing untracked `review/step4_*.md` artifacts were left untouched.

## Proof

Ran the delegated proof command after the audit-only `todo.md` update:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_.*|eastl_cpp_external_utility_frontend_basic_cpp)$' | tee test_after.log`.

Result: build completed successfully; CTest passed `886/886` matched tests.
Final proof log path: `test_after.log`.
