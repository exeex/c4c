# Calls Argument Sources Retirement Runbook

Status: Active
Source Idea: ideas/open/08_calls_argument_sources_retirement.md

## Purpose

Retire or shrink AArch64 `calls_argument_sources.cpp` now that call argument
source authority belongs to prepared call plans.

## Goal

The AArch64 argument-source helper surface becomes emission-oriented: prepared
plans carry semantic source facts, while target-local code only builds AArch64
operands and instructions from those facts.

## Core Rule

Do not replace `calls_argument_sources.cpp` with the same source-choice logic in
another AArch64 file. Source choice must stay in prepared planning or prepared
lookup records; AArch64 helpers may only translate complete facts into target
operands.

## Read First

- `ideas/open/08_calls_argument_sources_retirement.md`
- `src/backend/mir/aarch64/codegen/calls_argument_sources.cpp`
- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `src/backend/mir/aarch64/codegen/calls_dispatch_bridge.cpp`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- prepared call-plan source-selection records under `src/backend/mir`
- focused prepared-call and AArch64 call argument tests

## Current Targets

- Public helpers exposed from `calls_argument_sources.cpp`.
- `calls.hpp` declarations for argument-source helpers.
- Prepared call-plan facts that should replace target-local recovery.
- Build metadata and includes if helper files can be retired or reduced.

## Non-Goals

- Do not rework preservation republication; that belongs to idea `09`.
- Do not clean up dispatch materialization; that belongs to idea `12`.
- Do not change unrelated memory, prologue, variadic, or ABI lowering.
- Do not merge the whole calls file family; that belongs to idea `11`.
- Do not weaken call argument tests, diagnostics, or c_testsuite expectations.

## Working Model

- `calls_argument_sources.cpp` exists because AArch64 emission historically
  recovered where call arguments should come from.
- After prepared source selection and emission-only boundaries, most remaining
  helpers should either be deleted or reduced to target operand construction.
- Diagnostics should move with the authority they describe: prepared-layer
  source failures belong in prepared planning, not hidden in AArch64 emission.
- A smaller `calls.hpp` should expose only target-emission concepts that still
  need to be shared inside the AArch64 calls family.

## Execution Rules

- Audit before deleting: identify every caller and every public helper contract
  before changing the helper surface.
- Keep each code-changing packet narrow enough for build proof plus focused
  call argument tests.
- Prefer deleting duplicate reconstruction after prepared facts cover the same
  behavior.
- Update build metadata and includes in the same packet that retires a file or
  declaration.
- Treat helper-only moves, duplicate prepared/AArch64 selection logic, and
  expectation weakening as route failures.

## Ordered Steps

### Step 1: Audit Argument-Source Helper Ownership

Goal: classify every `calls_argument_sources.cpp` helper by its real owner.

Primary targets:

- `src/backend/mir/aarch64/codegen/calls_argument_sources.cpp`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- direct callers in the AArch64 calls family
- prepared call-plan source-selection APIs

Actions:

- List every public helper and declaration exposed through `calls.hpp`.
- For each helper, identify whether it chooses a semantic source, constructs an
  AArch64 operand, reports diagnostics, or merely adapts prepared facts.
- Map semantic source-choice helpers to the prepared records or lookup helpers
  that should own them.
- Record any missing prepared fact as a blocker for the relevant later step
  instead of preserving a target-local recovery path.

Completion check:

- The executor can name which helpers should be deleted, which should move to
  prepared planning, and which should remain as AArch64 operand construction.

### Step 2: Move Or Redirect Remaining Source Choice To Prepared Facts

Goal: eliminate target-local source-choice authority from argument-source
helpers.

Primary targets:

- prepared call-plan source-selection records and lookups
- `src/backend/mir/aarch64/codegen/calls_argument_sources.cpp`
- focused prepared-call tests

Actions:

- Redirect helpers that choose argument sources to consume prepared source
  records instead.
- Move diagnostics for incomplete or invalid source choices to the prepared
  layer when the failure is semantic rather than target-emission-specific.
- Add or tighten focused tests for any prepared fact that replaces AArch64
  recovery.
- Preserve AArch64-only operand formatting, register view, and address-building
  behavior locally.

Completion check:

- Prepared plans carry the facts formerly recovered target-locally, and
  AArch64 no longer duplicates the same source-selection rule.

### Step 3: Shrink The AArch64 Calls Surface

Goal: reduce `calls.hpp` and the argument-source helper API to target-emission
responsibilities.

Primary targets:

- `src/backend/mir/aarch64/codegen/calls.hpp`
- `src/backend/mir/aarch64/codegen/calls_argument_sources.cpp`
- callers in `calls_moves.cpp` and related calls files

Actions:

- Remove declarations for retired source-choice helpers.
- Inline or localize tiny adapters when they no longer represent shared
  ownership.
- Keep only helpers whose names and signatures communicate target operand
  construction or emission support.
- Verify callers use prepared records directly where that is the clearer
  contract.

Completion check:

- The exported calls helper surface is materially smaller and no longer
  suggests AArch64 owns semantic source selection.

### Step 4: Retire Or Reduce The Translation Unit

Goal: delete `calls_argument_sources.cpp` if possible, or leave it as a small
emission-only helper file.

Primary targets:

- `src/backend/mir/aarch64/codegen/calls_argument_sources.cpp`
- AArch64 backend build metadata
- includes and namespace-local helper definitions

Actions:

- Delete the file if all helpers have moved, been removed, or become local to
  their only caller.
- If deletion is not yet justified, reduce the file to clearly named
  emission-only helpers and document any remaining temporary fallback in the
  plan/todo layer.
- Update build metadata and includes with the same change that removes or
  renames the translation unit.
- Keep behavior-preserving moves separate from semantic authority changes when
  the blast radius is large.

Completion check:

- Build metadata has no stale entries, and any remaining argument-source file
  is small, emission-oriented, and not a semantic source-selection owner.

### Step 5: Validate Argument-Source Retirement

Goal: prove helper retirement without expectation rewrites or hidden source
selection.

Primary targets:

- focused prepared-call tests
- AArch64 call argument and instruction-dispatch tests
- representative backend/c_testsuite call cases selected by the supervisor

Actions:

- Run build or compile proof after each code-changing packet.
- Run focused tests for every helper family touched by the slice.
- Run representative backend call tests after API deletion, build metadata
  updates, or prepared-source redirection.
- Escalate to supervisor-selected broader validation before treating the
  retirement as complete.

Completion check:

- Focused call argument tests and representative backend tests pass, the
  argument-source helper surface is smaller and emission-oriented, and no
  unsupported markings, weaker contracts, or duplicate source-choice paths were
  introduced.
