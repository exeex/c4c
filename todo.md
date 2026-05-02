# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Parser-Owned Semantic Producers

## Just Finished

Step 2 - Migrate Parser-Owned Semantic Producers migrated
`src/frontend/sema/type_utils.cpp::type_binding_values_equivalent`.

The equality route now compares `TypeSpec` semantic identity through structured
carriers before rendered `tag` / qualifier strings:

- template parameter owner namespace, owner TextId, parameter index, and
  parameter TextId
- shared `record_def` identity when both sides carry parser record definitions
- namespace context plus `tag_text_id`, global qualification, and qualifier
  TextIds when complete type-name metadata is available
- existing `tpl_struct_origin_key` and `deferred_member_type_text_id`

Rendered `tag`, rendered qualifier segments, `tpl_struct_origin`, and
`deferred_member_type_name` remain compatibility fallback only when the matching
structured carrier family is absent on both sides.

Added focused stale-rendered-spelling coverage in
`frontend_parser_lookup_authority_tests`: `TypeSpec` equality now accepts
matching `record_def`, matching namespace/tag TextId metadata, and matching
template parameter metadata despite drifted rendered tags, and rejects
different structured TextIds even when rendered spelling matches.

## Suggested Next

Continue Step 2 with a parser-owned producer/consumer that still uses
`TypeSpec::tag` for semantic typedef or record identity despite carrying
`tag_text_id` or `record_def`. A good next target is the parser typedef-chain
or compatibility/type-comparison helpers in `parser/impl/core.cpp`, with a
stale-rendered-spelling parser/Sema test and the same focused proof subset.

## Watchouts

- Do not replace `TypeSpec::tag` with another rendered-string semantic field.
- Preserve diagnostics, dumps, mangling, ABI/link-visible text, and final
  spelling as payloads.
- Do not weaken tests, mark supported cases unsupported, or add named-case
  shortcuts.
- Keep downstream LIR/BIR/backend carrier gaps as separate follow-up ideas
  instead of broadening this runbook.
- Do not attempt `TypeSpec::tag` field deletion in Step 2; removal belongs to
  the later deletion/probe and removal steps.
- Treat a `TypeSpec::tag` deletion build as a temporary probe only until the
  runbook reaches the removal step.

## Proof

Step 2 delegated proof passed and wrote `test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|frontend_hir_tests|cpp_hir_.*template.*|cpp_positive_sema_.*deferred_nttp.*|cpp_positive_sema_.*consteval.*)$' | tee test_after.log`.

`git diff --check` passed.
