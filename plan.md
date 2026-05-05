# TypeSpec Tag Field Removal Metadata Migration Runbook

Status: Active
Source Idea: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Resumed After: ideas/closed/143_typespec_identity_normalization_boundary.md

## Purpose

Remove `TypeSpec::tag` as a cross-stage semantic identity field now that the
TypeSpec identity-normalization boundary has passed full-suite validation.

## Goal

Make `TypeSpec` carry semantic type identity through structured carriers such
as `TextId`, `record_def`, owner/member/template metadata, HIR owner keys,
declaration ids, or downstream type refs, while preserving rendered spelling
only for non-semantic uses.

## Core Rule

Do not replace `TypeSpec::tag` with another rendered-string semantic authority.
If a metadata-backed path still needs a string to find a type, add or thread
the missing structured carrier instead.

## Read First

- `ideas/open/141_typespec_tag_field_removal_metadata_migration.md`
- `ideas/closed/143_typespec_identity_normalization_boundary.md`
- `ideas/closed/139_parser_sema_rendered_string_lookup_removal.md`
- `ideas/closed/140_hir_legacy_string_lookup_metadata_resweep.md`
- `ideas/open/142_codegen_lir_aggregate_type_identity_metadata.md`
- `review/139_140_141_failure_attribution_review.md`

## Current Targets

- `TypeSpec` definition and constructors in parser/frontend type surfaces.
- Parser-owned semantic reads and writes of `TypeSpec::tag`.
- HIR compile-time, type lowering, builtin, function, and build paths named in
  the original deletion-probe failure clusters.
- Frontend/HIR fixtures and structured-metadata tests that still reference
  `TypeSpec::tag` directly.
- Downstream LIR/BIR/backend carrier gaps exposed only after the field is
  removed.

## Non-Goals

- Do not remove diagnostics, dump text, ABI/link spelling, mangling, or final
  emitted names merely because they are strings.
- Do not resurrect semantic lookup through rendered spelling under a different
  helper or field name.
- Do not silently expand into broad LIR/BIR/backend rewrites; split distinct
  downstream carrier boundaries into follow-up open ideas.
- Do not weaken tests, mark supported cases unsupported, or add named-case
  shortcuts.

## Working Model

- `TypeSpec::tag` is deletion debt, not a stable compatibility API.
- Parser and Sema producers should use `tag_text_id`, `record_def`,
  `QualifiedNameKey`, template-parameter metadata, deferred member metadata, or
  another explicit domain key for semantic identity.
- HIR consumers should use module-owned ids, declarations, owner keys, module
  type refs, or canonical handoff helpers instead of rendered TypeSpec
  spelling.
- Retained rendered text must be visibly classified as display, diagnostics,
  dumps, mangling, ABI/link spelling, final output, or no-metadata
  compatibility outside `TypeSpec::tag`.

## Execution Rules

- Start each packet with an inventory or deletion probe so the next blocker is
  observed rather than guessed.
- Keep each packet tied to one failure cluster and one proving subset.
- Prefer producer/handoff metadata repair over consumer-local rendered lookup.
- Preserve stale-rendered-spelling disagreement tests.
- When a blocker is a distinct downstream carrier boundary, record it as a new
  `ideas/open/*.md` initiative instead of broadening this runbook.

## Step 1: Reprobe TypeSpec Tag Removal Build Boundary

Goal: find the current first blockers after the normalization boundary closed.

Primary Target: `TypeSpec` declaration, constructors, and current
`TypeSpec::tag` references.

Actions:
- Inventory current `TypeSpec::tag` reads and writes with source locations.
- Run a controlled deletion or compile probe that removes the field enough to
  expose the first build blocker.
- Classify blockers as parser/Sema producer metadata, HIR consumer metadata,
  fixture/test debt, display-only spelling, or downstream carrier gap.

Completion Check:
- `todo.md` records the first blocker family, the command run, and the first
  implementation packet target.

## Step 2: Migrate Parser And Sema Semantic Tag Uses

Goal: remove parser-owned semantic dependence on rendered `TypeSpec::tag`.

Primary Target: parser AST/type construction, Sema type resolution, typedef,
record, enum, template, and deferred-member paths.

Actions:
- Replace semantic reads with existing `tag_text_id`, `record_def`,
  `QualifiedNameKey`, template parameter, or deferred member metadata.
- Add producer metadata where a producer still has only a rendered tag.
- Keep display spelling separate from semantic identity.

Completion Check:
- Focused parser/Sema build or test proof passes.
- No metadata-backed path introduces rendered-string semantic rediscovery.

## Step 3: Migrate HIR Semantic Tag Uses

Goal: remove HIR dependence on rendered TypeSpec spelling as lookup authority.

Primary Target: `compile_time_engine.hpp`, `hir_types.cpp`, `hir_ir.hpp`,
`hir_functions.cpp`, `hir_lowering_core.cpp`, `builtin.cpp`, and
`hir_build.cpp`.

Actions:
- Route type, record, template, member, and compile-time lookups through HIR or
  module structured carriers.
- Use normalized owner keys, declaration ids, module type refs, or canonical
  handoff helpers as primary identity.
- Keep rendered spelling only where it is display or explicit no-metadata
  compatibility.

Completion Check:
- HIR-focused tests and the relevant build target pass after the migrated
  cluster.
- Stale rendered spelling cannot override structured metadata.

## Step 4: Remove Fixture And Test Direct Field Debt

Goal: update tests and fixtures that directly reference `TypeSpec::tag` without
weakening behavior.

Primary Target: frontend/HIR unit tests, structured metadata fixtures, and
deletion-probe residual test helpers.

Actions:
- Rewrite fixture construction to use structured metadata carriers.
- Preserve or add tests where stale rendered spelling must lose to structured
  metadata.
- Avoid expectation rewrites that merely hide an unresolved semantic gap.

Completion Check:
- Focused frontend/HIR fixture tests pass.
- Test changes prove the migrated contract instead of lowering coverage.

## Step 5: Split Or Repair Downstream Carrier Gaps

Goal: handle downstream boundaries exposed by field removal without recreating
rendered semantic identity on `TypeSpec`.

Primary Target: LIR/BIR/backend/codegen call sites reached after frontend and
HIR blockers are cleared.

Actions:
- If the gap is within this idea's frontend/HIR migration scope, thread the
  structured carrier and prove it narrowly.
- If the gap is a distinct downstream metadata boundary, create a follow-up
  `ideas/open/*.md` with concrete reviewer reject signals.
- Do not reactivate idea 142 unless a fresh probe shows the same codegen/LIR
  aggregate identity boundary it owns.

Completion Check:
- `todo.md` records whether the boundary was repaired here or split into a
  separate source idea.

## Step 6: Delete TypeSpec Tag And Validate

Goal: finish the field removal and prove the migration is stable.

Primary Target: `TypeSpec` definition and all remaining compile/test fallout.

Actions:
- Remove `const char* tag` from `TypeSpec`.
- Remove obsolete compatibility shims that only existed to preserve the field.
- Run `cmake --build build --target c4cll`.
- Run focused tests for touched parser/Sema/HIR/downstream buckets.
- Escalate to supervisor-selected broad validation when the build and focused
  proofs are green.

Completion Check:
- `TypeSpec` has no `tag` field.
- `cmake --build build --target c4cll` passes.
- Required focused tests pass.
- Broad validation is green or remaining failures are split into follow-up
  ideas before lifecycle close.
