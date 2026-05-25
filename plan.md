# AArch64 Calls Block-Entry Preservation Republication Authority Checkpoint

Status: Active
Source Idea: ideas/open/02_aarch64_calls_emission_consolidation.md

## Purpose

Continue the AArch64 call-emission consolidation after the Step 5 closure
review for the prior stack-preservation lookup checkpoint rejected source-idea
closure.

## Goal

Remove or narrow the remaining AArch64-local preservation republication and
block-entry use-selection paths that duplicate shared prepared call-plan
authority, while keeping target-local code responsible only for AArch64
machine-node emission.

## Core Rule

Target-local AArch64 calls code may consume prepared preservation,
boundary-effect, move, and value-home facts to emit machine instructions. It
must not walk prepared call lists, scan BIR instructions, or decide preserved
value liveness when those decisions belong in shared prepared-call planning or
prepared lookup indexes.

## Latest Closure Review Finding

The Step 5 closure review after the selected prior stack-preservation lookup
route rejected source-idea closure.

The completed route replaced the selected reverse call-plan walk with prepared
lookup authority and the broader backend subset was green before review.

The source idea remains open because durable acceptance is broader than that
single preservation lookup route:

- `lower_value_moves` still performs an AArch64-local block-entry selection of
  prior callee-saved preservation republication effects by walking all prepared
  calls before the current block.
- `preserved_value_has_block_entry_non_call_use` still scans retained BIR block
  instructions and terminator/branch condition spelling to decide whether a
  preservation republication effect is needed at block entry.
- `make_callee_saved_preservation_home_republication_instruction` still
  synthesizes republication moves from selected preservation effects; that may
  be emission-only, but only after the selection authority is removed or
  proven to be prepared-owned.
- `calls_preservation.cpp` and `calls_moves.cpp` still expose helper
  signatures in `calls.hpp` whose boundaries mix prepared preservation facts,
  BIR-use checks, and AArch64 move emission.
- `calls_printing.cpp` remains a possible later source-idea target, but this
  checkpoint should not absorb printing unless the preservation boundary work
  exposes a small direct dependency.

Close-time regression guard generation was not needed because closure was
rejected before the close gate.

## Read First

- `ideas/open/02_aarch64_calls_emission_consolidation.md`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `src/backend/mir/aarch64/codegen/calls_preservation.cpp`
- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `src/backend/mir/aarch64/codegen/calls_printing.cpp`
- Shared prepared-call and lookup helpers under `src/backend/prealloc/`
- Focused backend preservation, prepared-call boundary, block-entry, byval, and
  AArch64 call tests under `tests/backend/mir/`

## Current Targets / Scope

- The block-entry preservation republication selection in `lower_value_moves`.
- `preserved_value_has_block_entry_non_call_use`.
- `make_callee_saved_preservation_home_republication_instruction` only as
  needed to keep the surviving boundary emission-only.
- `find_prior_preserved_value_for_value` only if the selected block-entry route
  proves it still blocks prepared-owned selection.
- Helper declarations in `calls.hpp` that expose BIR-use or preservation
  planning authority as AArch64 call-emission API.

## Non-Goals

- Do not work on `ideas/open/03_dispatch_responsibility_reduction.md`.
- Do not invent a new shared prepared-call API unless the selected blocker
  proves a required prepared fact is missing.
- Do not move AArch64-specific register, frame-slot, or instruction emission
  details into shared planning.
- Do not weaken unsupported or expected-output contracts.
- Do not change behavior solely to reduce line count.
- Do not treat printing relocation as part of this checkpoint unless the
  preservation boundary work directly exposes a small, coherent printing
  dependency.
- Do not revisit aggregate-address, local-frame publication, or prior
  stack-preservation lookup unless this block-entry republication route proves
  one of those completed routes still owns duplicate authority.

## Working Model

- Prefer prepared lookup indexes when target-local code only needs a prior
  prepared preservation fact.
- Prefer `prepare::plan_prepared_call_boundary_effects` output when
  target-local code only needs ordered preservation effects to emit.
- Move block-entry preserved-value-use eligibility into prepared data if the
  current AArch64 BIR scan is making a planning decision.
- Keep BIR inspection only for emission context, diagnostics, or identity
  checks that prepared data cannot represent.
- If the block-entry republication decision needs a fact that is genuinely
  missing, stop and record the missing prepared authority in `todo.md` instead
  of reconstructing the decision locally.
- A surviving helper boundary is acceptable only when its parameters describe
  AArch64 emission work, not preservation planning authority.

## Execution Rules

- Start with the exact block-entry preservation republication route.
- Delete local selection or use-reconstruction before adding replacement
  helper layers.
- Keep each code slice narrow enough for a fresh build plus focused backend
  proof.
- Escalate to `^backend_` after changing preservation-effect ordering,
  prepared lookup consumption, call-boundary effects, block-entry move
  behavior, or shared prepared-call behavior.
- Reject helper renames, expectation rewrites, and testcase-shaped shortcuts as
  progress.

## Step 1: Select The Block-Entry Republication Authority Leak

Goal: prove which part of block-entry preservation republication still owns
planning authority locally and whether the needed prepared fact already exists.

Primary targets:

- The `call_plans->calls` walk and `selected_preservation_effects` collection
  in `lower_value_moves`.
- `preserved_value_has_block_entry_non_call_use`.
- The `make_callee_saved_preservation_home_republication_instruction` call site
  for `PreparedMovePhase::BlockEntry`.

Actions:

- Map the block-entry republication decision to existing prepared preservation,
  boundary-effect, move-bundle, value-home, or lookup data.
- Decide whether local code should consume an existing prepared fact, whether a
  small shared prepared fact is missing, or whether the surviving work is
  emission-only.
- Record the selected deletion path, missing-prepared-fact blocker if any, and
  focused proof command in `todo.md`.

Completion check:

- `todo.md` names the selected block-entry preservation authority leak, the
  prepared replacement fact or missing-prepared-fact blocker, and the focused
  proof scope.

## Step 2: Remove The Selected Local Decision

Goal: replace the selected block-entry republication decision with
prepared-fact consumption or stop with a precise missing-prepared-fact blocker.

Actions:

- Delete the selected AArch64-local prepared-call walk or BIR-use scan from the
  block-entry republication path.
- Consume prepared preservation, boundary-effect, move-bundle, value-home, or
  lookup facts instead of rebuilding selection from retained BIR or raw call
  plan lists.
- Keep retained BIR checks only for instruction identity validation,
  diagnostics, or emission context.
- Tighten helper signatures so obsolete context parameters do not remain when
  prepared facts are sufficient.
- Update focused tests only to preserve or strengthen the prepared-authority
  contract.
- Run `cmake --build --preset default` plus the focused backend proof selected
  in Step 1.

Completion check:

- The selected block-entry preservation route no longer owns call-planning or
  preserved-value-use authority locally, and `test_after.log` records passing
  build plus focused proof.

## Step 3: Consolidate The Surviving Republication Helper Boundary

Goal: remove helper/API boundaries made obsolete by Step 2 or document why each
surviving preservation republication boundary is emission-only.

Actions:

- Remove obsolete declarations from `calls.hpp` and helper-local prototypes.
- Merge helper code back into a surviving owner when the helper no longer
  describes a real emission-only boundary.
- Keep `make_callee_saved_preservation_home_republication_instruction` only if
  it consumes prepared effect facts and emits AArch64 machine nodes without
  selecting preservation eligibility.
- Update build metadata only if a translation unit is retired.
- Run a fresh build and the focused backend proof after each coherent helper
  boundary change.

Completion check:

- The affected republication helper boundary is retired or explicitly
  emission-only, with build metadata and include graphs matching the new
  boundary.

## Step 4: Broader Backend Checkpoint

Goal: prove the latest block-entry republication checkpoint did not regress
adjacent preservation, call-boundary, byval, aggregate, local-frame
publication, prepared-call boundary, or printer behavior.

Actions:

- Run the supervisor-selected broader backend validation scope.
- Include focused AArch64 preservation, block-entry move, prepared-call
  boundary, local-frame/publication, byval, aggregate, and call printer tests.
- Include affected x86 shared-boundary tests if shared prepared-call behavior
  was touched.
- Record exact proof commands and results in `todo.md`.

Completion check:

- The broader checkpoint passes, or `todo.md` records a precise blocker tied
  to the active source idea.

## Step 5: Closure Review

Goal: decide whether the source idea is complete after this block-entry
republication checkpoint or whether another runbook checkpoint is needed.

Actions:

- Recheck all surviving `calls*.cpp`/`calls*.hpp` preservation, argument,
  dispatch, move, and printing helpers against the source idea acceptance
  criteria.
- Confirm target-local calls code no longer rederives decisions already
  present in shared prepared facts.
- Confirm surviving helper files have explicit emission-only ownership or are
  identified as the next checkpoint target.
- If durable remaining work exists, keep the idea open and request another
  runbook checkpoint instead of closing.

Completion check:

- Lifecycle state gives the supervisor an unambiguous close/keep-active
  decision.
