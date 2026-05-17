# Prealloc Regalloc Implementation Decomposition Runbook

Status: Active
Source Idea: ideas/open/274_prealloc_regalloc_implementation_decomposition.md

## Purpose

Split `src/backend/prealloc/regalloc.cpp` into focused implementation owners
while preserving register allocation behavior, move resolution, prepared facts,
and dump output.

## Goal

Make `regalloc.cpp` stop being the owner for unrelated prepared fact families
without changing allocation decisions or backend-visible output.

## Core Rule

This is a behavior-preserving decomposition. Do not change allocation
algorithms, register priorities, spill heuristics, ABI move semantics, stack
layout, or prepared output while extracting files.

## Read First

- `ideas/open/274_prealloc_regalloc_implementation_decomposition.md`
- `src/backend/prealloc/regalloc.cpp`
- `src/backend/prealloc/regalloc.hpp`
- `src/backend/prealloc/README.md`
- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/runtime_helpers.hpp`
- `src/backend/prealloc/stack_layout/stack_layout.hpp`
- `CMakeLists.txt` or the local build file that owns prealloc source lists

## Current Targets

- Keep the public regalloc phase entry stable.
- Prefer focused files under `src/backend/prealloc/regalloc/` when a family has
  enough implementation to justify a separate translation unit.
- Use narrow private helper headers only when multiple extracted files need the
  same internal types or lookup helpers.
- Preserve exact prepared dump text and existing test expectations.

## Non-Goals

- Do not redesign register allocation.
- Do not rework stack layout.
- Do not move prepared schema definitions.
- Do not split prepared printer output.
- Do not add new AArch64 or target capabilities.
- Do not rewrite tests or snapshots to accept changed behavior.

## Working Model

`regalloc.cpp` currently mixes several ownership families:

- value discovery and classification
- register class and width decisions
- interval ranking and physical assignment
- stack-slot fallback
- value-home publication
- call and return ABI move resolution
- phi and out-of-SSA move resolution
- pointer carrier tracking
- runtime-helper mapping
- spill and reload publication

Extract one family at a time. Each step should leave the tree buildable and
should prove that prepared output is unchanged.

## Execution Rules

- Move code mechanically first; only rename helpers when it clarifies ownership
  and preserves behavior.
- Keep shared internal state narrow. A helper header should expose only the data
  needed by multiple extracted files.
- Do not create a replacement catch-all file.
- Do not duplicate logic to avoid untangling dependencies.
- Update build wiring in the same step that adds a translation unit.
- After every code-changing step, run at least a fresh build or compile proof
  plus the delegated narrow backend/prealloc test subset.
- Escalate to broader backend/prealloc validation after several extraction
  steps or when shared helper state changes.

## Steps

### Step 1: Map Regalloc Ownership Boundaries

Goal: build a concrete extraction map before moving code.

Primary target: `src/backend/prealloc/regalloc.cpp`

Actions:

- Inspect top-level helper functions, local structs, and call sites in
  `regalloc.cpp`.
- Group helpers by the ownership families listed in the working model.
- Identify shared internal state that must remain in `regalloc.cpp` or move to
  a private helper header.
- Record the first low-risk extraction target in `todo.md` before code edits.

Completion check:

- `todo.md` names the first extraction packet and its proof command.
- No implementation files are changed by this mapping step unless the executor
  explicitly receives a code-changing packet.

### Step 2: Extract Pure Value Discovery and Classification Helpers

Goal: separate value enumeration and register-class/width decisions from the
allocator loop.

Primary target: `src/backend/prealloc/regalloc/values.cpp` and, if justified,
`src/backend/prealloc/regalloc/classification.cpp`

Actions:

- Move pure value discovery helpers into a focused translation unit.
- Move register class and width classification helpers only if they can be
  extracted without changing allocation policy.
- Introduce a narrow private header for shared type declarations only when
  required by both the allocator and extracted helpers.
- Update build wiring.

Completion check:

- The tree builds.
- Narrow backend/prealloc tests for prepared regalloc output pass with
  unchanged expectations.
- Allocation decisions and prepared dumps are unchanged.

### Step 3: Isolate Interval Ranking and Physical Assignment

Goal: make allocation proper a distinct owner from fact publication and move
resolution.

Primary target: `src/backend/prealloc/regalloc/allocator.cpp`

Actions:

- Move interval ranking, assignment, and stack-slot fallback logic into a
  focused allocator implementation.
- Keep the public phase entry in `regalloc.cpp` or an equivalent coordinator.
- Avoid changing comparator behavior, priority order, spill selection, or stack
  fallback policy.

Completion check:

- The build passes.
- Narrow regalloc/prealloc proof passes.
- A reviewer can see allocation-policy code without scanning call move,
  runtime-helper, or publication code.

### Step 4: Extract Value-Home Publication

Goal: move assigned-home publication out of allocator ownership.

Primary target: `src/backend/prealloc/regalloc/value_locations.cpp`

Actions:

- Move code that publishes prepared value locations and assigned homes.
- Keep the interaction with `value_locations.hpp` narrow and explicit.
- Do not change prepared schema data or printer output.

Completion check:

- Prepared value-location dumps are byte-for-byte unchanged.
- The build and delegated narrow prepared-output tests pass.

### Step 5: Extract ABI Call and Return Move Resolution

Goal: separate call/return ABI move resolution from allocation.

Primary target: `src/backend/prealloc/regalloc/call_moves.cpp`

Actions:

- Move call argument, return value, and ABI move-resolution helpers.
- Keep existing call-plan and storage-plan semantics unchanged.
- Avoid introducing target-specific shortcuts in shared regalloc code.

Completion check:

- Narrow call/return backend tests pass with unchanged prepared output.
- The extracted file owns ABI move resolution clearly.

### Step 6: Extract Phi and Out-of-SSA Move Resolution

Goal: isolate control-flow move publication from allocation policy.

Primary target: `src/backend/prealloc/regalloc/phi_moves.cpp`

Actions:

- Move phi/out-of-SSA move resolution helpers.
- Preserve existing ordering and move conflict handling.
- Keep dependencies on control-flow and liveness helpers explicit.

Completion check:

- Narrow phi/out-of-SSA prepared-output tests pass unchanged.
- No allocation ranking or ABI-call behavior changes appear in the diff.

### Step 7: Extract Pointer Carrier and Runtime-Helper Mapping

Goal: make special prepared facts independent of the allocator owner.

Primary targets:

- `src/backend/prealloc/regalloc/pointer_carriers.cpp`
- `src/backend/prealloc/regalloc/runtime_helper_mappings.cpp`

Actions:

- Move pointer-carrier tracking into its own owner.
- Move runtime-helper mapping into its own owner.
- Share only the minimal assigned-home or program-point data needed by these
  publishers.

Completion check:

- Runtime-helper and pointer-carrier prepared dumps are unchanged.
- Narrow tests pass without expectation edits.

### Step 8: Extract Spill and Reload Publication

Goal: separate spill/reload fact publication from allocation decisions.

Primary target: `src/backend/prealloc/regalloc/spill_reload.cpp`

Actions:

- Move spill and reload publication helpers.
- Keep spill heuristic and chosen homes unchanged.
- Do not rework stack layout.

Completion check:

- Spill/reload prepared dumps are unchanged.
- Narrow spill or stack-related backend/prealloc tests pass.

### Step 9: Consolidate Coordinator and Ownership Map

Goal: leave `regalloc.cpp` as a small coordinator and document where future
prepared facts belong.

Primary targets:

- `src/backend/prealloc/regalloc.cpp`
- `src/backend/prealloc/README.md` if it already carries prealloc ownership
  guidance

Actions:

- Remove dead helper declarations and local duplication left by extraction.
- Keep `regalloc.cpp` focused on phase orchestration and the public entry.
- Add or update concise ownership guidance only where this repo already keeps
  such guidance.

Completion check:

- `regalloc.cpp` is no longer a 3000+ line multi-family implementation file.
- The ownership map for new prepared facts is clear.
- No new catch-all file replaced the original monolith.

### Step 10: Broader Validation and Close Readiness

Goal: prove the decomposition preserved backend behavior.

Actions:

- Run a broader backend/prealloc validation command chosen by the supervisor.
- Compare prepared-output expectations; do not accept drift without a separate
  approved dump-format idea.
- Confirm the source idea completion criteria and reviewer reject signals.

Completion check:

- Broad validation passes.
- No expected prepared output was weakened.
- The supervisor has enough proof to request lifecycle closure review.
