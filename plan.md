# AArch64 Dispatch Family Contraction Audit Runbook

Status: Active
Source Idea: ideas/open/59_aarch64_dispatch_family_contraction_audit.md

## Purpose

Audit the remaining AArch64 `dispatch*` codegen family after the prepared
authority repair sequence and split the next cleanup work into precise
follow-up ideas.

## Goal

Produce a durable dispatch-family ownership classification and numbered
follow-up ideas without directly contracting implementation files.

## Core Rule

Classify ownership before proposing contraction. Do not merge, delete, or
rewrite dispatch implementation as part of this audit, and do not treat line
count or file count reduction as evidence of correct ownership.

## Read First

- `ideas/open/59_aarch64_dispatch_family_contraction_audit.md`
- `ideas/closed/47_aarch64_dispatch_edge_copies_prepared_authority_repair.md`
- `ideas/closed/48_aarch64_dispatch_publication_prepared_authority_repair.md`
- `ideas/closed/49_aarch64_dispatch_value_materialization_prepared_authority_repair.md`
- `ideas/closed/58_aarch64_prepared_authority_regression_recovery.md`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_lookup.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_lookup.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.hpp`

## Current Targets

- Audit all remaining AArch64 dispatch-family source and header files named in
  the source idea.
- Compare the family against the reference ARM codegen layout and the shared
  backend generation model.
- Classify each file or helper group as `fold-back`, `move-forward`,
  `keep-local`, `split-owner`, or `needs-more-evidence`.
- Create coherent follow-up idea files only after the classification evidence
  is specific enough to name owned files and ownership class.

## Non-Goals

- Do not perform the contraction in this plan.
- Do not repair idea 58 regression families here.
- Do not reopen ideas 47 through 49 unless the audit finds a concrete residual
  duplicate-authority path.
- Do not broaden into general AArch64 layout cleanup outside the dispatch
  family.
- Do not weaken tests, mark supported paths unsupported, or claim capability
  progress through expectation rewrites.

## Working Model

Ideas 47 through 49 moved the main duplicate prepared-authority paths out of
the AArch64 dispatch family. Idea 58 repaired the regression set that followed
that authority sequence. The remaining work is an ownership audit: decide which
dispatch files are still justified local target owners, which can fold back
into reference-style AArch64 owners, which need shared/BIR authority migration,
and which should become more precise non-dispatch target owners.

## Execution Rules

- Keep this plan audit-only unless the supervisor explicitly activates a
  follow-up contraction idea.
- Prefer evidence from closure notes, current helper behavior, caller/callee
  shape, and nearby reference layout over filename-level assumptions.
- Treat target-local scratch, clobber, diagnostics, instruction spelling, and
  emission sequencing as valid reasons to keep local ownership when supported
  by code evidence.
- Treat raw producer scans, fallback authority, or duplicate semantic recovery
  as reasons to propose shared/BIR migration only when the current code still
  demonstrates that ownership problem.
- Follow-up ideas must be small, named, and reviewable. Each one must identify
  owned files and whether it is mechanical fold-back, shared-authority
  migration, or target-owner split.
- Source idea files created as follow-ups must include concrete reviewer reject
  signals for testcase-shaped shortcuts, expectation downgrades, classification
  only claimed as capability progress, broad rewrites, and old failure modes
  hidden behind new helper names.

## Steps

### Step 1: Establish Audit Evidence Baseline

Goal: collect the source context needed to classify the dispatch family.

Primary target: idea closure notes plus current dispatch-family entry points.

Actions:

- Read the source idea and closure notes for ideas 47, 48, 49, and 58.
- Inventory the public helpers, local helper groups, callers, and callees across
  every `dispatch*` file named in the source idea.
- Identify the reference ARM codegen layout and the shared backend generation
  surfaces that provide comparison points.
- Record any current residual raw producer scans, local fallback authority, or
  duplicate semantic recovery candidates without changing implementation code.

Completion check:

- `todo.md` records the audited files, comparison surfaces, and any residual
  authority candidates that need closer classification.

### Step 2: Classify Each Dispatch File Or Helper Group

Goal: assign an evidence-backed ownership class to each dispatch file or
coherent helper group.

Primary target: the full AArch64 dispatch-family classification table.

Actions:

- Classify each file or helper group as `fold-back`, `move-forward`,
  `keep-local`, `split-owner`, or `needs-more-evidence`.
- For each classification, name the ownership evidence: target-local emission,
  scratch/clobber, diagnostics, instruction spelling, reference-layout match,
  shared semantic authority, or unresolved proof gap.
- Mark which files are expected to disappear, merge, stay local, or require
  shared/BIR authority work.
- Keep uncertain items as `needs-more-evidence` instead of forcing a
  contraction recommendation.

Completion check:

- `todo.md` contains or points to a durable classification table with an
  ownership class, evidence note, and expected disposition for every audited
  file or helper group.

### Step 3: Split Coherent Follow-Up Work

Goal: convert the classification table into bounded follow-up idea drafts.

Primary target: proposed follow-up idea set.

Actions:

- Group `fold-back` items into mechanical layout contraction ideas that can be
  proved without revalidating the whole dispatch family at once.
- Group `move-forward` items into shared/BIR authority migration ideas only
  when the audit found residual duplicate semantic ownership.
- Group `split-owner` items into target-owner split ideas with precise
  non-dispatch destinations.
- Leave `keep-local` items out of contraction follow-ups except where local
  naming or ownership documentation needs a separate audit-only idea.
- Leave `needs-more-evidence` items as narrow audit/proof ideas instead of
  implementation cleanup.

Completion check:

- `todo.md` records the proposed numbered follow-up list, each with owned
  files, ownership class, expected validation scope, and reject signals.

### Step 4: Materialize Follow-Up Source Ideas

Goal: create the accepted follow-up idea files under `ideas/open/`.

Primary target: new `ideas/open/` files for the next contraction or authority
migration slices.

Actions:

- Convert each accepted follow-up draft into a source idea file under
  `ideas/open/`.
- Include goal, why it exists, in-scope work, out-of-scope work, acceptance or
  completion criteria, and concrete reviewer reject signals.
- Keep follow-up scope small enough that one idea can be implemented without
  retesting the entire dispatch family.
- Do not edit implementation files while materializing follow-up ideas.

Completion check:

- Numbered follow-up source ideas exist under `ideas/open/`, each naming owned
  files and declaring whether it is mechanical fold-back, shared-authority
  migration, target-owner split, or evidence-gathering.

### Step 5: Closure Readiness Review

Goal: decide whether idea 59 itself can close after the audit artifacts and
follow-up ideas exist.

Actions:

- Confirm the classification table is durable and covers every file named in
  the source idea.
- Confirm follow-up ideas are specific enough to route without rediscovering
  ownership boundaries.
- Confirm no implementation or test changes were made under this audit plan.
- Confirm the final close note explains how the route avoids reintroducing raw
  producer scans or local fallback authority removed by ideas 47 through 49.

Completion check:

- The supervisor can route to plan-owner close review for idea 59, or
  `todo.md` names the remaining classification or follow-up split blocker.
