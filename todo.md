# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Parser-Owned Semantic Producers

## Just Finished

Step 2 - Migrate Parser-Owned Semantic Producers migrated
`src/frontend/parser/impl/core.cpp::Parser::are_types_compatible`.

Parser nominal type compatibility now uses structured identity before rendered
`TypeSpec::tag`: shared `record_def` identity when both sides carry record
definitions, then namespace context plus `tag_text_id`, global qualification,
and qualifier TextIds when complete metadata is available. Rendered tag
comparison remains explicit no-metadata compatibility only when neither side
has structured nominal identity.

Added focused stale-rendered-spelling coverage in
`frontend_parser_lookup_authority_tests`: `are_types_compatible` accepts shared
`record_def` or matching enum `tag_text_id`/namespace metadata despite stale
rendered tags, rejects mismatched structured record/TextId identity even when
rendered tags match, rejects one-sided structured metadata fallback, and keeps
rendered-only compatibility working.

## Suggested Next

Continue Step 2 by auditing another parser-owned semantic `TypeSpec::tag`
consumer with existing metadata, likely `Parser::resolves_to_record_ctor_type`
or parser struct/enum completeness helpers. Prefer `record_def` and
`tag_text_id`/structured visible-name metadata before rendered tag fallback,
with stale-rendered-spelling coverage and the same focused proof subset.

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

Step 2 delegated proof passed for the parser `are_types_compatible` packet
and wrote `test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|frontend_hir_tests|cpp_hir_.*template.*|cpp_positive_sema_.*deferred_nttp.*|cpp_positive_sema_.*consteval.*)$' | tee test_after.log`.

`git diff --check` passed.
