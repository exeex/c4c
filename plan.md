# Prepared Move Publication Indexing Prealloc Runbook

Status: Active
Source Idea: ideas/open/prepared-move-publication-indexing-prealloc.md

## Purpose

Move target-independent Prepared move and publication lookup indexing out of
`src/backend/mir/aarch64/codegen` and into `src/backend/prealloc` or a tightly
adjacent read-only prepared-consumer helper.

## Goal

Expose shared Prepared lookup records for call plans, address materialization
plans, move bundles, and value-home records so AArch64 consumes them without
target output changes and x86 has a plausible consumption path.

## Core Rule

Keep this work to read-only Prepared fact indexing. Do not move AArch64
instruction emission, register spelling, AAPCS64 policy, target addressing
legality, or assembly printing into prealloc.

## Read First

- `ideas/open/prepared-move-publication-indexing-prealloc.md`
- `src/backend/mir/aarch64/codegen`
- `src/backend/prealloc`
- x86 prepared-plan consumption surfaces, especially existing `ConsumedPlans`
  and prepared lowering code
- nearby tests that exercise calls, address materialization, move bundles, and
  value homes

## Scope

- Identify AArch64-owned Prepared indexing sites and the Prepared facts they
  consume.
- Add prealloc-owned or prealloc-adjacent read-only lookup helpers for the
  shared index shape.
- Convert AArch64 to consume the shared helpers without changing target output.
- Name the x86 consumption path around `ConsumedPlans` or equivalent prepared
  lowering surfaces.
- Add or update focused tests proving unchanged AArch64 lowering across more
  than one move/publication function shape.

## Non-Goals

- Do not rewrite call lowering or edge-copy emission behavior.
- Do not introduce a broad generic target abstraction.
- Do not move target register, ABI lane, addressing-legality, or assembly
  printing policy into prealloc.
- Do not weaken supported/unsupported test contracts or rewrite expectations to
  claim progress.
- Do not leave AArch64-named maps hidden behind a new helper name.

## Working Model

The new helper should be a Prepared consumer view over facts already owned by
`PreparedBirModule` and prealloc. It should build stable lookup records for:

- call-plan lookup
- address-materialization lookup
- move-bundle lookup
- value-home lookup

The helper must not emit machine instructions and must not encode AArch64
target policy. AArch64 should become one consumer of the lookup surface; x86
should have a named route to consume the same facts without copying the AArch64
context object.

## Execution Rules

- Start by mapping existing AArch64 construction and query sites before adding
  new helpers.
- Preserve target output while moving ownership of the index construction.
- Keep helper naming target-neutral and tied to Prepared/prealloc facts.
- Prefer small buildable steps: extraction, AArch64 conversion, x86 route
  proof, then test hardening.
- For code-changing steps, record fresh build or compile proof in `todo.md`.
- Acceptance proof must cover call-plan, address-materialization, move-bundle,
  and value-home lookup behavior, including nearby non-target cases rather than
  one narrow failing testcase.

## Step 1: Map Current AArch64 Prepared Indexing

Goal: Identify the current AArch64-owned lookup construction and query sites.

Primary target: `src/backend/mir/aarch64/codegen`

Actions:

- Inspect prepared-module traversal and dispatch helpers that build or query
  call-plan, address-materialization, move-bundle, and value-home indexes.
- Record the input Prepared facts each index consumes.
- Separate target-independent lookup construction from real AArch64 target
  emission and policy.
- Identify existing tests or snapshots that currently exercise each lookup
  family.

Completion check:

- `todo.md` names each AArch64 indexing site, its consumed Prepared facts, and
  the target-local code that must remain in AArch64.

## Step 2: Add a Target-Neutral Prepared Lookup Helper

Goal: Create the shared read-only helper surface in `src/backend/prealloc` or a
tightly adjacent prepared-consumer support module.

Primary target: `src/backend/prealloc`

Actions:

- Define target-neutral lookup record types or accessors for call plans,
  address materialization plans, move bundles, and value homes.
- Build the indexes from Prepared/prealloc-owned facts without depending on
  AArch64 register, ABI, or printing policy.
- Preserve existing missing-fact diagnostics or make them more explicit without
  hiding old failure modes behind a renamed abstraction.
- Keep the helper API narrow enough for AArch64 and x86 consumption.

Completion check:

- The helper builds independently of AArch64 policy and exposes read-only
  Prepared lookup results for the four required lookup families.

## Step 3: Convert AArch64 to Consume the Shared Helper

Goal: Remove AArch64-local ownership of target-independent Prepared indexing
while preserving output.

Primary target: `src/backend/mir/aarch64/codegen`

Actions:

- Replace AArch64-local index construction with the shared helper.
- Keep target emission, register naming, ABI lane policy, addressing legality,
  and assembly printing in AArch64.
- Preserve existing dispatch and lowering behavior at call boundaries,
  publications, moves, and value-home uses.
- Run a focused build or compile proof after conversion.

Completion check:

- AArch64 no longer owns the shared index construction, and focused proof shows
  unchanged target output for representative functions.

## Step 4: Name and Prove the x86 Consumption Path

Goal: Show that x86 can plausibly consume the helper without copying AArch64
context objects or policy.

Primary target: x86 prepared-plan consumption surfaces, especially
`ConsumedPlans` and prepared lowering code

Actions:

- Inspect current x86 prepared-plan consumption points.
- Name the exact x86 surface that can consume the new helper.
- Add the smallest compile-time or structural integration needed if the source
  idea supports it; otherwise document the consumption path in `todo.md`.
- Do not introduce a broad target abstraction.

Completion check:

- The run has concrete evidence that x86 can consume the helper through an
  existing prepared lowering surface, or records a precise blocker for
  lifecycle review.

## Step 5: Harden Focused Behavioral Proof

Goal: Prove this was a semantic ownership move, not testcase overfit.

Actions:

- Add or update tests covering more than one function shape using
  moves/publications.
- Include coverage for call-plan lookup, address-materialization lookup,
  move-bundle lookup, and value-home lookup.
- Include nearby non-target cases instead of only one known failing testcase.
- Run the supervisor-delegated proving subset and record results in
  `test_after.log` through executor workflow.

Completion check:

- Focused tests and proof logs show unchanged AArch64 lowering across the
  required lookup families, without expectation downgrades.

## Step 6: Completion Review

Goal: Decide whether the source idea is satisfied or whether a follow-up idea is
needed.

Actions:

- Verify the helper is target-neutral and not an AArch64 map with a new name.
- Verify AArch64 output is unchanged and target policy remains target-local.
- Verify x86 consumption evidence is present.
- Check for reviewer reject signals from the source idea.
- If a separate initiative is discovered, record it under `ideas/open/` through
  lifecycle authority instead of expanding this plan.

Completion check:

- The supervisor has enough evidence to accept the slice, request reviewer
  scrutiny, or route a follow-up lifecycle decision.
