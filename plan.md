# TypeSpec Tag Field Removal Metadata Migration Runbook

Status: Active
Source Idea: ideas/open/141_typespec_tag_field_removal_metadata_migration.md

## Purpose

Resume the parent `TypeSpec::tag` removal route after the codegen/LIR child
runbook returned the remaining deletion-probe boundary to frontend/HIR test
fixtures.

## Goal

Remove `TypeSpec::tag` as a cross-stage semantic identity field without
weakening parser, Sema, HIR, codegen, or backend behavior.

## Core Rule

Do not replace `TypeSpec::tag` with another rendered-string semantic lookup
authority. Rendered names may remain only as final spelling, diagnostics,
dumps, mangling, ABI/link text, or explicitly classified no-metadata
compatibility outside `TypeSpec::tag`.

## Read First

- `ideas/open/141_typespec_tag_field_removal_metadata_migration.md`
- `ideas/open/142_codegen_lir_aggregate_type_identity_metadata.md`
- `src/frontend/parser/ast.hpp`
- frontend/HIR tests that still read or write `TypeSpec::tag`
- `review/step5_lir_aggregate_identity_route_review.md`
- `review/step5_post_verifier_route_review.md`

## Current Targets

- Frontend/HIR test fixtures reported by the latest temporary deletion probe:
  `tests/frontend/frontend_parser_tests.cpp`,
  `tests/frontend/frontend_parser_lookup_authority_tests.cpp`,
  `tests/frontend/cpp_hir_template_type_arg_encoding_test.cpp`,
  `tests/frontend/cpp_hir_static_member_base_metadata_test.cpp`, and
  `tests/frontend/frontend_hir_lookup_tests.cpp`.
- Other frontend/HIR tests that still directly construct, read, or write
  `TypeSpec::tag` and therefore block compiling after the field is removed.
- The final `TypeSpec` field declaration in `src/frontend/parser/ast.hpp`.

## Non-Goals

- Reopening parked codegen/LIR idea 142 for parser or HIR test fixtures.
- Rewriting diagnostics, dump text, mangling, ABI/link spelling, or final
  output text merely because it is string-shaped.
- Weakening tests, marking supported cases unsupported, or deleting stale-tag
  disagreement coverage instead of preserving the same semantic contract with
  structured metadata.
- Broad parser, Sema, HIR, LIR, BIR, or backend rewrites outside the direct
  field-removal boundary.

## Working Model

- Use temporary deletion probes only to identify the next compile boundary; do
  not commit a broken deletion build.
- Treat test fixture `.tag` writes as compile blockers, not capability
  progress by themselves. Preserve the fixture's semantic assertion through
  `tag_text_id`, `record_def`, template/deferred-member metadata, or local
  compatibility helpers that compile with or without the legacy field.
- If a probe exposes a real frontend/HIR implementation residual, migrate the
  producer or consumer contract. If it exposes a downstream carrier gap, split
  a new child idea instead of expanding this runbook.
- When all compile residuals are cleared, delete `TypeSpec::tag` and prove the
  normal build.

## Execution Rules

- Keep packets small and buildable.
- Update `todo.md` after each packet with the migrated fixture or
  implementation family, proof command, and current deletion-probe boundary.
- For fixture-only packets, run `cmake --build --preset default` and the
  relevant frontend/HIR test subset selected by the supervisor.
- For the final field-deletion packet, run `cmake --build build --target c4cll`
  or the supervisor-selected equivalent build, then broader validation.

## Step 1: Classify Frontend/HIR Test Fixture Tag Uses

Goal: identify which remaining test `.tag` references are compile blockers,
intentional stale-rendered-spelling coverage, or no-metadata compatibility
fixtures.

Primary Target: `tests/frontend` `.tag` users and the deletion-probe failure
list.

Actions:
- Search `tests/frontend` for direct `TypeSpec::tag` reads and writes.
- Classify each use as stale-rendered-spelling disagreement coverage,
  no-metadata compatibility fallback coverage, final display spelling, or a
  removable compile-time fixture shortcut.
- Pick the first narrow fixture family that can be migrated without changing
  the tested semantic contract.

Completion Check:
- `todo.md` records the classified fixture surface and first executable
  migration target.
- No implementation behavior is changed in this step.

## Step 2: Migrate Fixture Helpers Off Direct Tag Access

Goal: make frontend/HIR tests compile without a `TypeSpec::tag` member while
preserving the same structured-metadata assertions.

Primary Target: helper-heavy frontend/HIR metadata tests.

Actions:
- Replace direct fixture `.tag` reads and writes with local helper wrappers
  only where the test intentionally covers legacy compatibility spelling.
- Prefer structured carriers in fixtures: `tag_text_id`, `record_def`,
  template parameter metadata, deferred owner/member metadata, or
  `QualifiedNameKey`.
- Keep stale-rendered-spelling disagreement tests meaningful; do not erase the
  stale spelling setup unless the assertion no longer depends on it.

Completion Check:
- Migrated test families compile with and without the legacy field present.
- Relevant frontend/HIR tests pass.

## Step 3: Re-run The TypeSpec Tag Deletion Probe

Goal: confirm the first boundary has moved past frontend/HIR fixtures or
classify the next owned residual.

Primary Target: temporary `TypeSpec::tag` deletion build.

Actions:
- Temporarily remove or disable `TypeSpec::tag` in
  `src/frontend/parser/ast.hpp`.
- Run the supervisor-selected build probe.
- Record the first remaining compile boundary and ownership in `todo.md`.
- Restore the probe edit unless this step becomes the final accepted deletion
  packet.

Completion Check:
- `todo.md` records the probe result.
- The worktree returns to a normal buildable state after any non-final probe.

## Step 4: Migrate Frontend/HIR Implementation Residuals

Goal: clear any remaining parser, Sema, or HIR implementation dependence on
`TypeSpec::tag` exposed after fixture cleanup.

Primary Target: the first frontend/HIR implementation residual reported by the
deletion probe.

Actions:
- Migrate semantic lookup to structured carriers such as `TextId`,
  `record_def`, template parameter metadata, deferred member metadata,
  `QualifiedNameKey`, HIR declaration ids, record owner keys, or module type
  refs.
- Preserve rendered spelling only at diagnostics, dumps, final output,
  mangling, ABI/link text, or explicitly named no-metadata compatibility
  boundaries.
- Add or preserve focused tests that prove stale rendered spelling does not
  decide semantic lookup when structured metadata exists.

Completion Check:
- The migrated implementation family no longer reads `TypeSpec::tag` as
  semantic authority when structured metadata is available.
- `cmake --build --preset default` and the supervisor-selected focused subset
  pass.

## Step 5: Delete TypeSpec::tag And Prove The Field Removal

Goal: remove the field from `TypeSpec` and accept only a buildable,
non-overfit final state.

Primary Target: `src/frontend/parser/ast.hpp` and any residual compile errors
from removing the field.

Actions:
- Delete the `TypeSpec::tag` field.
- Fix remaining compile errors by removing direct field access or routing
  semantics through existing structured metadata.
- Do not restore a broad rendered-string compatibility field on `TypeSpec`.

Completion Check:
- `cmake --build build --target c4cll` or the supervisor-selected equivalent
  build passes with `TypeSpec::tag` removed.
- Focused frontend/HIR metadata tests pass.

## Step 6: Validate And Close Or Split

Goal: decide whether the parent source idea is complete or whether a newly
exposed boundary needs its own follow-up idea.

Primary Target: validation logs and lifecycle state.

Actions:
- Confirm retained rendered spelling is classified outside `TypeSpec::tag`.
- Run broader validation selected by the supervisor.
- If the field is removed and validation passes, close this source idea.
- If a new non-frontend/HIR carrier boundary appears, create a narrower open
  child idea and retire or park this runbook without claiming closure.

Completion Check:
- The lifecycle state reflects either source-idea closure or a documented
  follow-up boundary.
