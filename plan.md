# AArch64 Calls Preservation Reconstruction Helper Authority Checkpoint

Status: Active
Source Idea: ideas/open/02_aarch64_calls_emission_consolidation.md

## Purpose

Continue the AArch64 call-emission consolidation after the Step 5 closure
review for the local-frame publication checkpoint rejected source-idea closure.

## Goal

Remove or narrow the remaining AArch64-local preservation reconstruction paths
that duplicate shared prepared call-plan authority, while keeping target-local
code responsible only for AArch64 emission.

## Core Rule

Target-local AArch64 calls code may consume prepared preservation, move, and
boundary-effect facts to emit machine nodes. It must not rediscover
preservation liveness, prior-call dominance, or call-boundary effect identity
when those facts are already available from shared prepared-call planning or
prepared lookup indexes.

## Latest Closure Review Finding

The Step 5 closure review after the local-frame publication backend checkpoint
rejected closure.

The selected local-frame helper chain is complete for this route: the retained
`CallInst::arg_abi` and `CallInst::arg_types` publication reads targeted by
the previous checkpoint were removed, and the broader backend subset was green
before review.

The source idea remains open because the durable acceptance criteria are
broader than the local-frame publication route:

- `calls_preservation.cpp` still owns preservation reconstruction helpers that
  walk prepared call lists, inspect BIR block contents, and decide later-use or
  prior-preserved-value eligibility near emission.
- `calls_moves.cpp` still has preservation-effect selection and
  republication/population routing that should be checked against prepared
  boundary-effect authority before being treated as emission-only.
- `calls_printing.cpp` still owns call and call-boundary printing/effect
  spelling, which the source idea explicitly lists as a remaining possible
  move out of target-local call emission.
- The AArch64 calls family still spans multiple helper translation units; no
  closure-quality file-retirement or responsibility boundary review has been
  completed for preservation and printing.

Close-time regression guard generation was not needed because closure was
rejected before the close gate.

## Read First

- `ideas/open/02_aarch64_calls_emission_consolidation.md`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls_preservation.cpp`
- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `src/backend/mir/aarch64/codegen/calls_printing.cpp`
- `src/backend/mir/aarch64/codegen/calls_dispatch_bridge.cpp`
- `src/backend/mir/aarch64/codegen/calls_dispatch_bridge.hpp`
- Shared prepared-call and lookup helpers under `src/backend/prealloc/`
- Focused backend preservation, prepared-call boundary, byval, and AArch64 call
  tests under `tests/backend/mir/`

## Current Targets / Scope

- `find_prior_preserved_value_for_call_argument`.
- `find_prior_preserved_value_for_value`.
- `find_prior_stack_preserved_value_before_instruction`.
- `preserved_value_has_later_non_call_use`.
- `preserved_value_has_block_entry_non_call_use`.
- Callee-saved preservation home republication and population helpers.
- Preservation-effect filtering in `calls_moves.cpp`.
- Helper signatures in `calls.hpp` that expose preservation reconstruction as
  target-local call-emission API.

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
- Do not revisit aggregate-address or local-frame publication unless a
  preservation dependency proves that route still owns duplicate authority.

## Working Model

- Prefer prepared lookup indexes when the target-local code only needs to find
  a prior preserved value already selected by shared planning.
- Prefer `prepare::plan_prepared_call_boundary_effects` output when the
  target-local code only needs ordered preservation effects to emit.
- Keep BIR inspection only for emission context, diagnostics, or identity
  checks that prepared data cannot represent.
- If a preservation decision needs a fact that is genuinely missing, stop and
  record the missing prepared authority in `todo.md` instead of reconstructing
  the decision locally.
- A surviving helper boundary is acceptable only when its parameters describe
  AArch64 emission work, not preservation planning authority.

## Execution Rules

- Start with one exact preservation reconstruction helper or caller.
- Delete local reconstruction before adding any replacement helper.
- Keep each code slice narrow enough for a fresh build plus focused backend
  proof.
- Escalate to `^backend_` after changing preservation-effect ordering,
  prepared lookup consumption, call-boundary effects, or shared prepared-call
  behavior.
- Reject helper renames, expectation rewrites, and testcase-shaped shortcuts as
  progress.

## Step 1: Select One Preservation Reconstruction Authority Leak

Goal: choose the next duplicate-authority preservation path to remove and
prove whether the needed prepared fact already exists.

Primary targets:

- `find_prior_preserved_value_for_call_argument`.
- `find_prior_preserved_value_for_value`.
- `find_prior_stack_preserved_value_before_instruction`.
- `preserved_value_has_later_non_call_use`.
- `preserved_value_has_block_entry_non_call_use`.
- Preservation-effect filtering in `lower_before_call_moves` and
  `lower_after_call_moves`.

Actions:

- Identify the exact helper or call site that still reconstructs preservation
  authority near AArch64 emission.
- Map that decision to existing prepared preservation, boundary-effect, or
  lookup data.
- Select one coherent preservation target for the next executor packet.
- Record the selected target, expected deletion path, and proof command in
  `todo.md`.

Completion check:

- `todo.md` names one preservation reconstruction authority leak, the prepared
  replacement fact or missing-prepared-fact blocker, and the focused proof
  scope.

## Step 2: Remove The Selected Preservation Decision

Goal: replace the selected retained preservation reconstruction with
prepared-fact consumption or stop with a precise missing-prepared-fact blocker.

Actions:

- Delete the selected local preservation reconstruction from the target-local
  path.
- Consume prepared preservation, boundary-effect, or lookup facts instead of
  rebuilding them from BIR scans or call-list walks.
- Keep retained BIR checks only for instruction identity validation,
  diagnostics, or emission context.
- Tighten helper signatures so obsolete context parameters do not remain when
  prepared facts are sufficient.
- Update focused tests only to preserve or strengthen the prepared-authority
  contract.
- Run `cmake --build --preset default` plus the focused backend proof selected
  in Step 1.

Completion check:

- The selected preservation path no longer owns call-planning authority
  locally, and `test_after.log` records passing build plus focused proof.

## Step 3: Consolidate The Preservation Helper Boundary

Goal: remove helper/API boundaries made obsolete by Step 2 or document why each
surviving preservation boundary is emission-only.

Actions:

- Remove obsolete declarations from `calls.hpp` and helper-local prototypes.
- Merge helper code back into a surviving owner when the helper no longer
  describes a real emission-only boundary.
- Update build metadata only if a translation unit is retired.
- Keep edits limited to preservation reconstruction and direct call-boundary
  emission ownership.
- Run a fresh build and the focused backend proof after each coherent helper
  boundary change.

Completion check:

- The affected preservation helper boundary is retired or explicitly
  emission-only, with build metadata and include graphs matching the new
  boundary.

## Step 4: Broader Backend Checkpoint

Goal: prove the latest preservation-authority checkpoint did not regress
adjacent call, byval, aggregate, local-frame publication, prepared-call
boundary, or printer behavior.

Actions:

- Run the supervisor-selected broader backend validation scope.
- Include focused AArch64 preservation, prepared-call boundary,
  local-frame/publication, byval, aggregate, and call printer tests.
- Include affected x86 shared-boundary tests if shared prepared-call behavior
  was touched.
- Record exact proof commands and results in `todo.md`.

Completion check:

- The broader checkpoint passes, or `todo.md` records a precise blocker tied
  to the active source idea.

## Step 5: Closure Review

Goal: decide whether the source idea is complete after this preservation
checkpoint or whether another runbook checkpoint is needed.

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
