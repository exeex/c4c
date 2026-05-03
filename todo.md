# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries migrated the HIR
`value_args.cpp` deletion-probe residuals in the member lookup
owner/template-argument binding family around current owner resolution,
template type-arg binding, deferred member owner lookup, and `is_reference`
trait owner recovery. The migrated paths now use `record_def`, `tag_text_id`,
owner-key metadata, `template_param_text_id`, and text-id binding maps before
explicit field-detected rendered-spelling compatibility fallback. Added
focused coverage in `cpp_hir_value_args_residual_structured_metadata`.

## Suggested Next

Continue Step 4 with the next supervisor-selected residual family. The current
first emitted deletion-probe residuals are now in parser expression
display/mangling and constructor/member owner spelling paths in
`src/frontend/parser/impl/expressions.cpp`, starting around current local
`append_type_mangled_suffix_local` lines 33-36 and later constructor/member
fallback sites around 1480, 1573, 1786-1790, and 2011.

## Watchouts

- Do not replace `TypeSpec::tag` with another rendered-string semantic field.
- Preserve diagnostics, dumps, mangling, ABI/link-visible text, and final
  spelling as payloads.
- Step 4 is a probe, not the final field removal. Do not commit a broken
  deletion build.
- Classify each deletion-probe failure as parser/HIR-owned,
  compatibility/display/final spelling, or downstream metadata gap.
- Do not weaken tests, mark supported cases unsupported, or add named-case
  shortcuts.
- `resolve_typedef_type_chain` is now semantically cleared for structured
  typedef-chain lookup. Its remaining rendered-tag fallback is compatibility
  only, disabled whenever structured typedef lookup identity is present, and
  uses a field-detection helper so the Step 4 deletion probe classifies later
  residuals first.
- `resolve_struct_like_typedef_type` is now semantically cleared for
  structured typedef lookup. Its remaining rendered-tag fallback is
  compatibility only, disabled whenever structured typedef lookup identity is
  present, and uses a field-detection helper so the Step 4 deletion probe
  classifies later residuals first.
- `resolve_struct_member_typedef_if_ready` readiness is now semantically
  cleared for structured owner metadata. Its no-metadata rendered-tag fallback
  is explicit and field-detection guarded, and a structured `tag_text_id` miss
  no longer falls back to a stale rendered tag in the non-template owner path.
- `resolve_struct_member_typedef_if_ready` origin member-typedef
  template-binding is now semantically cleared for structured template
  parameter metadata. Its no-metadata rendered-tag fallback is explicit through
  `find_template_typedef_binding`, and structured owner mismatch does not fall
  back to stale rendered names.
- `resolve_struct_member_typedef_if_ready` nested deferred-member readiness is
  now semantically cleared for structured owner/deferred-member metadata. Its
  no-metadata rendered-tag fallback is explicit and field-detection guarded,
  and nested record-def metadata can recurse without any rendered tag.
- `resolve_struct_member_typedef_if_ready` template-owner realized lookup is
  now semantically cleared for structured owner metadata. Its no-metadata
  rendered-tag fallback is explicit and field-detection guarded, and realized
  template owners can resolve inherited member typedefs through the HIR struct
  `tag_text_id`/owner index path.
- `recover_template_struct_identity_from_tag` is now semantically cleared for
  structured template/record owner metadata. Its no-metadata rendered-tag
  fallback is explicit and field-detection guarded.
- `resolves_to_record_ctor_type` is now semantically cleared for structured
  record-constructor metadata. Its no-metadata rendered-tag fallback is
  explicit and field-detection guarded.
- `register_synthesized_typedef_binding`, `register_tag_type_binding`, and
  builtin `__true_type`/`__false_type` setup are now semantically cleared as
  parser/core producers. `tag_text_id` carries the typedef/tag identity, and
  rendered spelling assignment is explicit field-detected compatibility only.
- `visible_type_head_name` is now semantically cleared for structured visible
  type spelling. Its rendered-tag return is explicit field-detected
  compatibility/display fallback after structured visible metadata misses.
- `type_spec_structured_record_definition` is now semantically cleared for
  structured record lookup by `tag_text_id`. Its rendered-tag lookup is
  explicit field-detected no-metadata compatibility fallback.
- Prior deletion probe residuals remain clear for the targeted direct
  `TypeSpec::tag` reads around former
  `src/frontend/parser/impl/types/types_helpers.hpp:117` and `145`.
- Deletion probe residuals from this packet no longer include the targeted
  direct reads around former
  `src/frontend/parser/impl/types/types_helpers.hpp:489`, `1190`,
  `1233-1253`, and `1965-1968`.
- Declarations current-struct incomplete-type checks are now semantically
  cleared for structured visible type metadata. No-metadata rendered-tag
  fallback remains explicit and field-detection guarded.
- Declarations incomplete-type diagnostics and local conversion-operator
  mangling now derive display names from `record_def`, `tag_text_id`, or
  `template_param_text_id` before field-detected rendered fallback.
- Top-level typedef/template-param producer paths around the previous
  `base_ts.tag` assignments now set `tag_text_id` first and preserve rendered
  final spelling only through field-detected compatibility assignment.
- Deletion probe residuals from this packet no longer include the targeted
  direct reads/assignments around former
  `src/frontend/parser/impl/declarations.cpp:928-933`, `1287`, `2835`,
  `2853`, and `2926-2931`; the local mangling residual around former
  `315-338` is also cleared.
- This packet added
  `/tmp/c4c_typespec_tag_deletion_probe_step4_declarations_residual.log`.
- HIR `value_args.cpp` member lookup owner resolution now prefers structured
  owner metadata from `record_def`, `tag_text_id`, and owner-key indexes before
  no-metadata rendered compatibility fallback.
- HIR `value_args.cpp` template type-argument binding now uses
  `template_param_text_id`/`tag_text_id` with `tpl_bindings_by_text` before
  no-metadata rendered-name binding fallback, so stale rendered template
  parameter tags no longer drive semantic binding.
- HIR `value_args.cpp` deferred member owner lookup and `is_reference` trait
  owner recovery now use structured owner metadata before field-detected
  no-metadata rendered compatibility fallback.
- Deletion probe residuals from this packet no longer include the targeted
  direct reads around former
  `src/frontend/hir/impl/templates/value_args.cpp:386`, `401-404`,
  `414-437`, `466-493`, `763-789`, and `870-871`.
- This packet added
  `/tmp/c4c_typespec_tag_deletion_probe_step4_value_args_residual.log`.

## Proof

Executor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_lookup_tests|cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp|frontend_hir_tests|cpp_hir_.*)$"' > test_after.log 2>&1`

Result: command exited 0. The build passed, and CTest passed 95 of 95
delegated tests, increasing the focused subset with new
`cpp_hir_value_args_residual_structured_metadata` coverage for the migrated
HIR value-args metadata. `test_after.log` is the canonical proof log.

Regression guard:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`

Result: command exited 0. Guard passed with `passed=94 failed=0 total=94`
before and `passed=95 failed=0 total=95` after.

Deletion probe:

Temporarily removed `TypeSpec::tag` from `src/frontend/parser/ast.hpp`, ran
`bash -lc 'cmake --build --preset default' >
/tmp/c4c_typespec_tag_deletion_probe_step4_value_args_residual.log 2>&1`,
and restored the temporary edit. The probe moved past the targeted
`value_args.cpp` direct reads around former lines `386`, `401-404`,
`414-437`, `466-493`, `763-789`, and `870-871`.

Result: command exited 1 as expected for the controlled deletion probe. The
first emitted errors are now in
`src/frontend/parser/impl/expressions.cpp` around current local mangling lines
`33-36`, followed by constructor/member display fallback reads around `1480`,
`1573`, `1786-1790`, and `2011`. The normal delegated proof above was rerun
green after restoring the temporary edit.
