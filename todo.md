# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate HIR Type And Record Consumers

## Just Finished

Step 3 - Migrate HIR Type And Record Consumers migrated function template
specialization TypeSpec argument matching in
`src/frontend/hir/compile_time_engine.hpp::InstantiationRegistry::select_function_specialization`.
The matcher now compares structured nominal TypeSpec identity before rendered
`TypeSpec::tag`: shared `record_def` wins first, complete namespace plus
`tag_text_id` plus qualifier TextIds can match next, and one-sided or
mismatched structured nominal metadata rejects the specialization without
falling through to rendered spelling. Rendered `tag` comparison remains the
explicit no-structured-metadata compatibility path.

Focused coverage in `frontend_hir_lookup_tests` adds direct compile-time
registry fixtures proving a specialization matches through shared `record_def`
despite stale rendered TypeSpec tags, and proving mismatched `record_def`
identity does not fall back to a matching rendered tag.

## Suggested Next

Continue Step 3 with another bounded HIR `TypeSpec::tag` consumer where
structured owner metadata is already present. A good next packet is a narrow
template/type route that still indexes by rendered `tag`, such as
`src/frontend/hir/impl/templates/templates.cpp` base/specialization lookup or a
caller in `src/frontend/hir/impl/templates/value_args.cpp` that can pass
`record_def`, `tag_text_id`, owner keys, or template metadata instead of only
rendered owner spelling.

## Watchouts

- Do not replace `TypeSpec::tag` with another rendered-string semantic field.
- Preserve diagnostics, dumps, mangling, ABI/link-visible text, and final
  spelling as payloads.
- The deferred member typedef resolver still intentionally uses rendered
  `resolve_struct_member_typedef_type(tag, ...)` for no-metadata compatibility
  and for concrete realized template-origin base traversal after origin
  materialization.
- Function specialization TypeSpec argument matching still intentionally uses
  rendered `tag` comparison when both sides lack structured nominal metadata.
  Template parameter name lookup remains keyed by rendered parameter names in
  `TypeBindings`; migrating that boundary needs a separate structured template
  parameter binding packet.
- The default preset used for this packet does not register
  `frontend_hir_tests`; focused coverage for this route was therefore added to
  `frontend_hir_lookup_tests`, which the delegated regex compiles and runs.
- Do not weaken tests, mark supported cases unsupported, or add named-case
  shortcuts.
- Keep downstream LIR/BIR/backend carrier gaps as separate follow-up ideas
  instead of broadening this runbook.
- Do not attempt `TypeSpec::tag` field deletion in Step 3; removal belongs to
  the later deletion/probe and removal steps.
- Treat a `TypeSpec::tag` deletion build as a temporary probe only until the
  runbook reaches the removal step.

## Proof

Step 3 delegated proof passed with:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|frontend_hir_lookup_tests|cpp_hir_.*template.*|cpp_positive_sema_.*deferred_nttp.*|cpp_positive_sema_.*consteval.*)$' | tee test_after.log`.

Result: build passed and 60/60 selected tests passed. Proof log:
`test_after.log`. `git diff --check` passed.
