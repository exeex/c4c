# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate Parser-Owned Semantic Producers

## Just Finished

Step 2 - Migrate Parser-Owned Semantic Producers migrated
`src/frontend/parser/impl/core.cpp::Parser::resolves_to_record_ctor_type`.

Record-constructor classification now checks structured metadata before
rendered `TypeSpec::tag`: direct `record_def`, structured typedef resolution
from `tag_text_id`, template struct primary lookup by TextId/context, and
record definitions matched by parser-owned record TextId metadata. Rendered tag
fallback remains explicit compatibility only when the incoming `TypeSpec` has
no `tag_text_id` carrier.

Added focused stale-rendered-spelling coverage in
`frontend_parser_lookup_authority_tests`: record-constructor classification
uses structured typedef/record metadata before a stale rendered tag, rejects
rendered fallback after a structured TextId miss, preserves no-metadata
rendered fallback, and accepts direct `record_def` metadata even with different
rendered spelling.

## Suggested Next

Continue Step 2 by auditing parser struct/enum completeness helpers or another
parser-owned semantic `TypeSpec::tag` consumer with existing `record_def` or
`tag_text_id` metadata. Prefer structured record/visible-name metadata before
rendered tag fallback, with stale-rendered-spelling coverage and the same
focused proof subset.

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

Step 2 delegated proof passed for the parser
`resolves_to_record_ctor_type` packet and wrote `test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_lookup_authority_tests|frontend_hir_tests|cpp_hir_.*template.*|cpp_positive_sema_.*deferred_nttp.*|cpp_positive_sema_.*consteval.*)$' | tee test_after.log`.

`git diff --check` passed.
