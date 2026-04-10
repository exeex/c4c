# x86_64 Backend Bring-Up After BIR Cutover

Status: Active
Source Idea: ideas/open/44_x86_64_backend_bringup_after_bir_cutover.md

## Purpose

Finish the current x86_64 bring-up lane without spending more engineering time
feeding testcase-shaped legacy lowering. Shift the active work from adding new
bounded recoveries to shrinking the legacy matcher surface and forcing the tree
toward reusable shared-BIR lowering.

## Goal

Turn the current `lir_to_bir` path from a growing collection of
`try_lower_minimal_*` source-shaped recognizers into a smaller, better-bounded
transition layer:

- remove exact rendered-IR text matchers
- stop adding new testcase-shaped seams by default
- concentrate ownership in reusable normalization and structured lowering
- leave x86_64 coverage intact for already-recovered cases while paying down
  the matcher debt

## Core Rule

Do not add a new testcase-specific matcher when the same effort could be spent
on:

- CFG normalization
- structured memory / aggregate / call lowering
- shrinking an existing `legacy-lowering` seam

Any new bounded matcher added during this plan must be justified as a temporary
bridge and must identify the generic rule that is supposed to replace it.

## Current Scope

- shared `lir_to_bir` cleanup, especially under
  `src/backend/lowering/lir_to_bir/memory.cpp` and
  `src/backend/lowering/lir_to_bir/calls.cpp`
- reduction of the `legacy-lowering` surface inside
  `src/backend/lowering/lir_to_bir.cpp`
- preservation of already-recovered x86_64 source-backed routes owned by idea
  44

## Non-Goals

- do not revive legacy backend IR or LLVM asm rescue
- do not widen this plan into a full multi-target lowering rewrite
- do not absorb the parked `00040.c`, `00051.c`, or rv64 select-route work into
  this cleanup lane
- do not chase unrelated EASTL/parser failures

## Working Model

- treat `try_lower_to_bir(...)` as the formal API surface
- treat `try_lower_to_bir_legacy(...)` and all `.phase = "legacy-lowering"`
  paths as shrink targets, not expansion targets
- pay down one coherent matcher family at a time
- prefer instruction-structured matching over rendered-text matching
- preserve existing owned x86_64 coverage while shrinking matcher debt

## Execution Phases

Phase 1: Legacy Inventory

- map the remaining `print_llvm()` / `kExpectedModule` / exact-text checks
- group them by subsystem: memory, calls, aggregates, control flow
- identify which ones are easy structural rewrites versus which require a real
  generic rule first

Phase 2: Matcher Cleanup

- remove the lowest-risk exact rendered-text matchers first
- replace them with structured block/instruction matching without changing
  owned behavior
- keep validation targeted and local to the owned seams

Phase 3: Path Consolidation

- identify legacy seams that should move behind CFG normalization or a generic
  lowering helper
- reduce the visible `legacy-lowering` surface in
  `try_lower_to_bir_with_options(...)`
- only after that resume choosing new x86 failing cases

Phase 4: Regression Guard

- keep already-recovered x86 source-backed cases green
- use targeted tests as the default guard for cleanup slices
- use broad-suite runs as periodic checkpoints, not as the driver for every
  matcher-removal slice while the known unrelated red buckets remain

## Execution Rules

- start each slice from the highest-priority cleanup item in `todo.md`
- no new `print_llvm()` / full-fragment matcher may be introduced
- when a matcher is removed, preserve or tighten the targeted regression that
  proves the owned seam still works
- if a cleanup slice exposes a truly separate initiative, park it under
  `ideas/open/` instead of mutating this runbook ad hoc

## Step 1: Inventory Remaining Legacy Matchers

Goal: produce a short, actionable list of the remaining exact-text and strongly
testcase-shaped matchers in the active shared-BIR path.

Concrete actions:

- inventory `print_llvm()` / `kExpectedModule` / exact equality checks in
  `lir_to_bir/*`
- record which ones have already been converted to structured matching
- identify the next 2-4 lowest-risk cleanup targets

Completion check:

- the remaining cleanup queue is explicit and prioritized

## Step 2: Remove Low-Risk Exact-Text Matchers

Goal: eliminate the easiest remaining rendered-IR text matchers without
changing owned x86_64 behavior.

Concrete actions:

- convert one matcher family at a time from rendered-text to structured checks
- prefer `memory.cpp` batches first, then `calls.cpp`
- revalidate with the narrowest lowering, pipeline, and source-backed route
  tests for the owned seam

Completion check:

- the chosen matcher family no longer depends on exact rendered LLVM text
- owned x86_64 route coverage for those seams still passes

## Step 3: Reduce Legacy Dispatcher Pressure

Goal: make `try_lower_to_bir_with_options(...)` less dependent on the growing
legacy matcher list.

Concrete actions:

- identify seams that can move behind generic normalization or common helpers
- collapse duplicate or near-duplicate `try_lower_minimal_*` families where the
  same structural rule can own them
- document the remaining irreducible legacy seams as explicit transition debt

Completion check:

- at least one legacy seam family is merged, generalized, or removed
- the active queue is no longer "add the next bounded matcher"

## Step 4: Resume Case Recovery Only After Cleanup Turns The Corner

Goal: return to x86 failing-case recovery only after the matcher debt stops
growing.

Concrete actions:

- use refreshed after-logs only once the cleanup queue for the active family is
  under control
- continue to park separate initiatives instead of widening idea 44 ad hoc

Completion check:

- new case recovery resumes from a cleaner lowering base, not from further
  legacy expansion
