# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Parser-Owned Semantic Producers

## Just Finished

Step 2 - Migrate Parser-Owned Semantic Producers migrated
`src/frontend/parser/impl/core.cpp` typedef-chain resolution.

`Parser::resolve_typedef_type_chain` and
`Parser::resolve_struct_like_typedef_type` now resolve typedef references by
`TypeSpec::tag_text_id` first. The metadata-backed route checks qualified
TextId paths, namespace-context structured typedef keys, and visible typedef
bindings before considering rendered spelling. Rendered `TypeSpec::tag`
fallback remains explicit no-metadata compatibility only when the incoming
`TypeSpec` has no `tag_text_id` carrier.

Added focused stale-rendered-spelling coverage in
`frontend_parser_lookup_authority_tests`: a typedef query with
`tag = "StaleRenderedAlias"` and `tag_text_id = StructuredAlias` resolves to
the structured alias target, while the same query without `tag_text_id` still
uses the rendered compatibility fallback.

## Suggested Next

Continue Step 2 by auditing another parser-owned semantic `TypeSpec::tag`
consumer with existing metadata, likely `Parser::are_types_compatible` for
record/enum identity or `resolves_to_record_ctor_type` for record constructor
classification. Prefer `record_def` and `tag_text_id`/structured visible-name
metadata before rendered tag fallback, with stale-rendered-spelling coverage
and the same focused proof subset.

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

Step 2 delegated proof passed for the parser `core.cpp` typedef-chain packet
and wrote `test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|frontend_hir_tests|cpp_hir_.*template.*|cpp_positive_sema_.*deferred_nttp.*|cpp_positive_sema_.*consteval.*)$' | tee test_after.log`.

`git diff --check` passed.
