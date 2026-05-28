# AArch64 Dispatch Lookup Wrapper Fold-Back Runbook

Status: Active
Source Idea: ideas/open/60_aarch64_dispatch_lookup_wrapper_fold_back.md

## Purpose

Fold thin AArch64 dispatch lookup, producer, and publication-result wrappers
back into their direct prepared, BIR query, or publication-query owners where
they no longer carry target-specific policy.

## Goal

Reduce dispatch-family surface area without changing lookup semantics,
recursive value materialization, edge-copy fallback behavior, select-chain
discovery, or join-copy source reconstruction.

## Core Rule

This is a mechanical fold-back only: call sites must use existing prepared or
shared query authority directly, or local inline convenience that does not
introduce new target-specific semantic lookup policy.

## Read First

- `ideas/open/60_aarch64_dispatch_lookup_wrapper_fold_back.md`
- `src/backend/mir/aarch64/codegen/dispatch_lookup.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_lookup.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- Publication-result helper call sites for `instruction_result_value` and
  `instruction_result_value_ref`

## Current Targets

- Thin prepared lookup wrappers:
  - `prepared_named_value_id`
  - `prepared_value_id`
  - `find_value_home`
- Producer wrappers over `mir::query`:
  - `find_same_block_binary_producer`
  - `find_same_block_named_producer`
  - `evaluate_same_block_integer_constant`
- Publication result-value helpers:
  - `instruction_result_value`
  - `instruction_result_value_ref`

## Non-Goals

- Do not move semantic lookup, producer, or publication authority into AArch64
  code.
- Do not change recursive value materialization decisions.
- Do not change edge-copy fallback behavior, select-chain discovery, or
  join-copy source reconstruction.
- Do not broadly merge unrelated dispatch files into `dispatch.cpp`.
- Do not weaken supported-path tests or rewrite expectations as proof of
  progress.

## Working Model

`dispatch_lookup.*` and `dispatch_producers.*` currently expose wrappers around
prepared/shared query owners. This runbook removes or localizes only wrappers
whose behavior is already fully owned elsewhere. Any helper that still embeds
target-specific policy should be left in place and recorded in `todo.md` for a
later source idea, not silently expanded here.

## Execution Rules

- Inspect each wrapper's call graph before editing it.
- Prefer replacing public wrapper calls with the existing owner API at the call
  site.
- Use local inline convenience only when it is clearly mechanical and private
  to the immediate owner.
- Do not reimplement raw BIR scans at call sites.
- If public dispatch headers shrink, run at least a fresh build and focused
  AArch64 backend coverage; escalate to the supervisor for broader regression
  guard if the blast radius reaches shared call sites or multiple public
  headers.
- Record any discovered non-mechanical authority in `todo.md` rather than
  changing the source idea.

## Ordered Steps

### Step 1: Inventory Wrapper Use

Goal: classify every in-scope wrapper by owner, caller set, and whether it is
mechanical.

Primary target:
`src/backend/mir/aarch64/codegen/dispatch_lookup.*` and
`src/backend/mir/aarch64/codegen/dispatch_producers.*`.

Actions:
- Find all declarations, definitions, and call sites for the thin prepared
  lookup wrappers.
- Find all declarations, definitions, and call sites for the producer wrappers
  over `mir::query`.
- Find all declaration, definition, and call sites for publication result-value
  helpers.
- Identify the direct prepared, BIR query, or publication-query owner each
  wrapper delegates to.
- Mark wrappers that are not mechanical as blocked for later source ideas.

Completion check:
- `todo.md` records which wrappers are mechanical, which owner API should
  replace each one, and any wrappers excluded because they still carry
  target-specific policy.

### Step 2: Fold Thin Prepared Lookup Wrappers

Goal: replace mechanical prepared lookup wrappers with direct prepared or
shared query usage.

Primary target:
`prepared_named_value_id`, `prepared_value_id`, and `find_value_home`.

Actions:
- Replace call sites with the direct prepared/shared owner API identified in
  Step 1.
- Remove now-unused wrapper declarations and definitions.
- Keep call-site behavior unchanged for missing homes and optional lookup
  results.
- Do not introduce new AArch64 semantic lookup decisions while removing the
  wrappers.

Completion check:
- The code builds far enough to prove declarations and definitions are
  consistent, and the affected value-home call sites no longer depend on public
  dispatch lookup wrappers unless Step 1 proved a wrapper is non-mechanical.

### Step 3: Fold Producer Query Wrappers

Goal: replace mechanical producer wrappers over `mir::query` with direct query
or prepared/shared owner usage.

Primary target:
`find_same_block_binary_producer`, `find_same_block_named_producer`, and
`evaluate_same_block_integer_constant`.

Actions:
- Replace mechanical call sites with direct query or owner calls.
- Remove unused public wrapper declarations and definitions.
- Preserve same-block producer behavior exactly; this step must not migrate
  same-block semantic authority.
- Leave any non-mechanical recursive source-discovery behavior for idea 61
  rather than broadening this route.

Completion check:
- The code builds far enough to prove the producer wrapper surface shrank, and
  focused call sites still compile without new raw BIR scans in AArch64
  dispatch code.

### Step 4: Fold Publication Result-Value Helpers

Goal: replace mechanical publication result-value helpers with the correct
publication-query or immediate local owner.

Primary target:
`instruction_result_value` and `instruction_result_value_ref`.

Actions:
- Locate publication result consumers and identify the true owner of result
  extraction.
- Replace wrapper calls with direct owner API or private local convenience.
- Remove unused wrapper declarations and definitions.
- Preserve behavior for publication result extraction, comparison and FP
  materialization hooks, and dispatch hook consumers.

Completion check:
- Publication result extraction call sites compile, no public helper remains
  solely as a forwarding wrapper, and behavior-sensitive consumers still use
  the same result facts as before.

### Step 5: Prune Public Surfaces And Prove The Slice

Goal: finish the fold-back without leaving forwarding helpers or stale public
  declarations.

Primary target:
dispatch lookup and producer headers plus affected AArch64 backend tests.

Actions:
- Remove unused includes, declarations, definitions, and namespace exports from
  the affected dispatch-family files.
- Confirm no folded helper survives under a new dispatch-family name.
- Run a fresh build or compile proof selected by the supervisor.
- Run focused AArch64 backend coverage for lookup and publication-result paths.
- Ask the supervisor to escalate to `c4c-regression-guard` if public headers
  were removed or coverage crosses multiple backend buckets.

Completion check:
- Build and focused proof are green, `todo.md` records the exact commands and
  log names, and any remaining wrapper authority is explicitly tied to a later
  open idea rather than hidden in this runbook.
