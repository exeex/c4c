# TypeSpec Identity Normalization Boundary Runbook

Status: Active
Source Idea: ideas/open/143_typespec_identity_normalization_boundary.md
Supersedes Active Runbook: ideas/open/141_typespec_tag_field_removal_metadata_migration.md

## Purpose

Park the TypeSpec field-removal route and repair the upstream identity contract
that caused repeated consumer-side canonicalization fixes after
`119a6ef0430fbb4967cfd79f7d6b3a12ad5f6bc8`.

## Goal

Make TypeSpec semantic identity normalized at production and parser-to-HIR
handoff boundaries so consumers stop rediscovering record, typedef, owner, and
tag identity through scattered rendered-tag fallback.

## Core Rule

Do not add another consumer-local tentative rendered-tag lookup when structured
metadata exists. Normalize once, then consume a domain key.

## Read First

- `ideas/open/143_typespec_identity_normalization_boundary.md`
- `review/139_140_141_failure_attribution_review.md`
- `ideas/closed/139_parser_sema_rendered_string_lookup_removal.md`
- `ideas/closed/140_hir_legacy_string_lookup_metadata_resweep.md`
- `ideas/open/141_typespec_tag_field_removal_metadata_migration.md`
- commits since `119a6ef0430fbb4967cfd79f7d6b3a12ad5f6bc8`

## Current Targets

- TypeSpec producer and typedef resolution surfaces in
  `src/frontend/parser/impl/types/base.cpp` and related parser helpers.
- HIR/module identity handoff helpers in `src/frontend/hir` and
  `src/frontend/sema/consteval.*`.
- Shared aggregate/type identity helpers in `src/codegen/shared/llvm_helpers.hpp`.
- Remaining broad validation failures attributed to 139/140/141 identity
  drift, especially record/layout lookup, parser cast disambiguation, and
  template owner identity.

## Non-Goals

- Do not delete `TypeSpec::tag` in this runbook.
- Do not reopen 139 or 140 as broad cleanup routes; use them as historical
  inputs.
- Do not weaken tests or rewrite expectations to reduce the failure count.
- Do not remove diagnostics, dumps, mangling, final output, or ABI/link text.

## Working Model

- `TypeSpec::tag_text_id` is only authoritative inside the text table that
  produced it.
- Parser producers should attach the best local identity immediately:
  `record_def`, namespace-qualified `tag_text_id`, template parameter metadata,
  deferred member metadata, or another explicit domain key.
- Parser-to-HIR lowering should canonicalize parser-local identity into HIR
  module identity such as `HirRecordOwnerKey`, `ModuleDeclLookupKey`,
  `LinkNameId`, or module type refs.
- Consumers may use rendered spelling only as classified no-metadata
  compatibility or final/display spelling.

## Execution Rules

- Keep each packet tied to one identity family and one proving subset.
- Prefer extracting a shared normalization helper over adding new fallback
  branches in consumers.
- Preserve stale-rendered-spelling disagreement tests.
- After each packet, update `todo.md` with the normalized boundary touched, the
  tests run, and the remaining failure family.

## Step 1: Inventory Post-119a6ef Consumer Repairs

Goal: identify duplicated canonicalization and tentative rendered-tag lookup
patterns.

Primary Target: commits and code touched after
`119a6ef0430fbb4967cfd79f7d6b3a12ad5f6bc8`.

Actions:
- Inspect recent commits that changed consteval record lookup, codegen
  aggregate lookup, owner-key lookup, and parser typedef TypeSpec resolution.
- List consumer-local direct `tag_text_id` lookups against foreign
  `TextTable` instances.
- Classify each lookup as normalized identity, no-metadata compatibility, or
  residual route drift.

Completion Check:
- `todo.md` records the duplicated patterns and the first normalization target.

## Step 2: Normalize Parser TypeSpec Producer Identity

Goal: make parser-produced TypeSpecs carry the right source identity before
consumer lookup begins.

Primary Target: parser type producers and typedef resolution.

Actions:
- Ensure typedef resolution preserves source typedef identity unless a stronger
  structured identity such as `record_def` applies.
- Audit struct/union/enum/template/deferred-member producers for stale stored
  TypeSpec metadata overwriting source carriers.
- Add or preserve focused frontend/HIR metadata tests.

Completion Check:
- Focused parser/HIR producer metadata tests pass.
- No new consumer-local rendered fallback is introduced.

## Step 3: Centralize HIR Record Owner Canonicalization

Goal: replace duplicated cross-table record owner repair with one shared
Module-aware normalization path.

Primary Target: consteval, HIR lowering, and shared codegen aggregate helpers.

Actions:
- Define or consolidate a helper that converts TypeSpec aggregate identity into
  a canonical HIR owner key using module-owned text tables.
- Route record-layout and aggregate consumers through that helper.
- Keep rendered tag lookup only as explicit no-metadata compatibility.

Completion Check:
- Group A/B-style record/layout tests remain green or improve.
- Tests with stale rendered tags still reject wrong structured identity.

## Step 4: Normalize Parser Cast And Template Owner Identity

Goal: address the remaining parser disambiguation and template-owner failures
without reintroducing rendered-string authority.

Primary Target: generated c-style-cast disambiguation tests and EASTL/template
specialization incomplete-type failures.

Actions:
- Trace where owner-dependent `typename` and template-member cast targets lose
  their structured owner.
- Thread the normalized owner/type identity through the parser/Sema handoff.
- Prove with same-feature generated matrix tests, not a named-case shortcut.

Completion Check:
- Group D parser disambiguation tests improve.
- EASTL/template owner incomplete-type tests improve or are reclassified with
  a precise blocker.

## Step 5: Retire Consumer-Side Compatibility Debt

Goal: remove or demote fallback helpers made unnecessary by normalization.

Primary Target: helpers named legacy/fallback/compatibility that still decide
semantic identity in metadata-backed paths.

Actions:
- Delete duplicated local fallback paths after their consumers use normalized
  identity.
- Keep compatibility helpers only where no metadata exists and name them
  accordingly.
- Add reviewer-facing notes in `todo.md` for any compatibility path retained.

Completion Check:
- The diff reduces consumer-local rediscovery code instead of moving it.
- Retained string paths are visibly non-semantic or no-metadata compatibility.

## Step 6: Broad Validation And 141 Resume Decision

Goal: decide whether TypeSpec field removal can safely resume.

Primary Target: full validation and lifecycle state.

Actions:
- Run supervisor-selected broad validation, usually
  `ctest --test-dir build -j 8 --output-on-failure`.
- Compare remaining failures against
  `review/139_140_141_failure_attribution_review.md`.
- If normalized identity is stable, switch lifecycle back to idea 141.
- If a distinct downstream boundary remains, create a narrower open idea
  instead of expanding this runbook.

Completion Check:
- Validation shows no new failures relative to this idea's accepted baseline.
- Lifecycle state records whether 141 should resume or another child idea is
  needed.
