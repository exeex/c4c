# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repair Parser-to-Sema Metadata Handoff Gaps

## Just Finished

Step 4 removed one Sema rendered `TypeSpec::tag` gate in
`hir::resolve_type`: typedef substitution now tries the intrinsic
`template_param_*` key and `tag_text_id` carriers before requiring rendered
`TypeSpec::tag` spelling, and only enters the rendered `type_bindings`/
name-mirror compatibility path when no intrinsic carrier is present. Added
`test_consteval_type_binding_resolve_uses_tag_text_id_without_tag` to prove
Sema consteval `sizeof(T)` substitution works with `tag_text_id` and a null
rendered tag.

## Suggested Next

Continue Step 4 review after the next review trigger. A likely next narrow
inspection route is Sema `lookup_record_layout` in `consteval.cpp`: it still
uses `env.struct_defs` keyed by rendered `TypeSpec::tag` for HIR-backed
`sizeof`/`alignof`, but `ConstEvalEnv` currently exposes only the rendered
HIR struct map, not the HIR `struct_def_owner_index`/owner-key carrier.

## Watchouts

- This packet intentionally did not edit HIR, LIR, BIR, backend, `plan.md`, or
  `ideas/open`.
- `lookup_record_layout` appears to need an env-carried structured HIR record
  owner/index before its rendered `TypeSpec::tag` lookup can be removed; adding
  that carrier crosses into HIR env construction and should be delegated
  explicitly if selected next.
- Existing untracked `review/step4_*.md` artifacts were left untouched.

## Proof

Ran the delegated proof command after the Sema `resolve_type` route removal:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|cpp_positive_sema_.*|eastl_cpp_external_utility_frontend_basic_cpp)$' | tee test_after.log`.

Result: build completed successfully; CTest passed `886/886` matched tests.
Final proof log path: `test_after.log`.
