# TypeSpec Tag Field Removal Metadata Migration Runbook

Status: Active
Source Idea: ideas/open/141_typespec_tag_field_removal_metadata_migration.md

## Purpose

Execute the explicit metadata migration needed before `TypeSpec::tag` can stop
being a cross-stage semantic identity field.

## Goal

Remove `TypeSpec::tag` as semantic lookup authority and represent type identity
with existing or new structured carriers such as `TextId`, `record_def`,
template parameter metadata, owner/member keys, HIR record owner keys, or
downstream type refs.

## Core Rule

Do not replace `TypeSpec::tag` with another rendered-string semantic key.
Rendered spelling may remain only as diagnostics, dumps, mangling,
ABI/link-visible text, final output, or explicitly named no-metadata
compatibility outside `TypeSpec::tag`.

## Read First

- `ideas/open/141_typespec_tag_field_removal_metadata_migration.md`
- `src/frontend/parser/ast.hpp`
- `src/frontend/hir/hir_ir.hpp`
- First failure clusters from the prior deletion experiment:
  `src/frontend/hir/impl/compile_time/compile_time_engine.hpp`,
  `src/frontend/hir/hir_types.cpp`, `src/frontend/hir/hir_functions.cpp`,
  `src/frontend/hir/hir_lowering_core.cpp`,
  `src/frontend/hir/impl/expr/builtin.cpp`, and
  `src/frontend/hir/hir_build.cpp`

## Current Targets

- Parser-owned `TypeSpec::tag` producers and consumers.
- HIR uses of `TypeSpec::tag` as record, enum, typedef, template parameter,
  member, or record-layout identity.
- HIR lowering/compile-time paths that still need `tag_text_id`, `record_def`,
  owner keys, template parameter metadata, deferred member metadata, or module
  structured lookup carriers.
- Tests proving structured metadata wins when rendered type spelling drifts.

## Non-Goals

- Removing diagnostics, dumps, mangling, ABI/link spelling, final emitted
  names, or other display text merely because it is string-shaped.
- Replacing `TypeSpec::tag` with a renamed rendered-string semantic field.
- Broad LIR, BIR, backend, or codegen rewrites. Create separate open ideas for
  downstream carrier gaps exposed by the frontend/HIR migration.
- Weakening tests, marking supported cases unsupported, or adding named-case
  shortcuts.

## Working Model

- Classify every `TypeSpec::tag` read/write before deleting the field.
- Migrate one semantic producer/consumer family at a time.
- Preserve or add producer metadata before changing consumers.
- Keep no-metadata compatibility routes narrow, named, and tested.
- Use a temporary deletion build only as a probe to find remaining owned
  clusters; do not commit a broken deletion.

## Execution Rules

- Keep implementation packets small and buildable.
- Update `todo.md` after each executor packet with the exact migrated family,
  proof command, and residual boundaries.
- Source idea edits should be rare; routine progress belongs in `todo.md`.
- Add focused parser/HIR tests for stale rendered type spelling where a route
  is migrated.
- For code-changing packets, run `cmake --build --preset default` plus the
  supervisor-selected focused parser/HIR subset. Use broader validation at the
  field-removal checkpoint and closure.

## Step 1: Inventory TypeSpec Tag Surfaces

Goal: classify every `TypeSpec::tag` read/write before behavior changes.

Primary Target: parser, Sema, HIR, and lowering files that touch
`TypeSpec::tag`.

Actions:
- Search all `TypeSpec::tag`, `.tag`, `tag_text_id`, `record_def`,
  template-parameter metadata, deferred member metadata, and related helpers.
- Classify each `TypeSpec::tag` surface as semantic identity, final/display
  spelling, diagnostics/dumps, mangling/link spelling, compatibility payload,
  local scratch, or missing metadata.
- Identify which semantic surfaces already have replacement carriers and which
  need producer metadata first.
- Name the first concrete migration target.

Completion Check:
- `todo.md` records the classified surface map and the first executable
  migration target.
- No field deletion or broad behavior change is required in this step.

## Step 2: Migrate Parser-Owned Semantic Producers

Goal: ensure parser-produced `TypeSpec` values carry structured identity before
HIR/Sema consumers stop reading `tag`.

Primary Target: parser type construction and declaration/type helpers.

Actions:
- Replace parser-owned semantic `TypeSpec::tag` authority with existing
  `tag_text_id`, `record_def`, `template_param_*`, `deferred_member_*`,
  `QualifiedNameKey`, or another domain key where available.
- Add missing producer metadata only when the producer already has the
  semantic identity.
- Keep rendered spelling for diagnostics, dumps, and explicit compatibility.

Completion Check:
- Covered parser-owned semantic type identities have non-`tag` metadata.
- Focused parser/Sema tests prove structured metadata wins over stale rendered
  type spelling for at least one migrated family.

## Step 3: Migrate HIR Type And Record Consumers

Goal: remove metadata-backed HIR lookup dependence on rendered `TypeSpec::tag`
for record, enum, typedef, template, and member identity.

Primary Target: HIR lowering, type resolution, compile-time, and record-layout
consumers.

Actions:
- Convert HIR consumers to `TextId`, `record_def`, HIR record owner keys,
  module declaration lookup keys, member keys, template metadata, or module
  type refs where available.
- Keep rendered `tag`-derived text only as compatibility or output spelling.
- Add focused HIR tests for stale rendered type spelling with valid structured
  metadata.

Completion Check:
- Covered HIR semantic lookup paths no longer use rendered `TypeSpec::tag` as
  authority when structured metadata is present.
- Remaining HIR `tag` uses are classified with explicit next steps or
  compatibility rationale.

## Step 4: Probe Field Removal And Split Boundaries

Goal: use a controlled deletion/build probe to expose remaining field-removal
clusters without committing broken code.

Primary Target: `TypeSpec::tag` declaration and first build failure clusters.

Actions:
- Temporarily delete or disable `TypeSpec::tag` and run
  `cmake --build --preset default`.
- Record remaining failures by ownership and semantic category.
- Revert only the probe edit, then route remaining work:
  parser/HIR-owned failures stay in this runbook; downstream LIR/BIR/backend
  metadata gaps become separate `ideas/open/` follow-ups.

Completion Check:
- `todo.md` records the probe result, remaining owned clusters, and any new
  follow-up ideas created.
- The worktree is returned to a buildable state after the probe.

## Step 5: Remove TypeSpec Tag Field

Goal: delete `TypeSpec::tag` after semantic producer/consumer routes have been
migrated or explicitly split.

Primary Target: `TypeSpec` definition and all remaining compile errors caused
by field removal.

Actions:
- Remove `const char* tag` from `TypeSpec`.
- Fix remaining parser/HIR/frontend compile errors by using structured
  carriers or explicit output/compatibility payloads.
- Do not reintroduce broad rendered-string semantic storage under another name.
- Preserve diagnostics, dumps, ABI/link text, mangling, and final spelling.

Completion Check:
- `cmake --build --preset default` passes with no `TypeSpec::tag` field.
- Focused parser/HIR tests cover migrated structured-vs-rendered disagreement
  routes.

## Step 6: Validate And Close Boundaries

Goal: close the runbook only after validation and remaining boundaries are
explicit.

Primary Target: validation logs, `todo.md`, and any required follow-up ideas.

Actions:
- Confirm remaining rendered spelling connected to type specs is classified as
  diagnostics, display, dump, mangling, ABI/link text, final output, or
  explicit no-metadata compatibility.
- Run supervisor-selected broader validation after field removal.
- Create separate open ideas for downstream carrier gaps that outlive this
  runbook.
- Confirm reviewer reject signals from the source idea do not apply.

Completion Check:
- `cmake --build --preset default` passes.
- Broader validation selected by the supervisor passes.
- Follow-up metadata blockers are represented as open ideas instead of hidden
  inside this runbook.
