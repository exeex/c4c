# AArch64 Calls Local Frame Publication Helper Authority Checkpoint

Status: Active
Source Idea: ideas/open/02_aarch64_calls_emission_consolidation.md

## Purpose

Continue the AArch64 call-emission consolidation after the Step 5 closure
review for the aggregate-address publication checkpoint rejected closure.

## Goal

Remove the remaining local-frame address publication-helper decisions that
still read retained `bir::CallInst::arg_abi` or `bir::CallInst::arg_types`
instead of consuming prepared call-plan facts.

## Core Rule

Target-local AArch64 calls code may inspect retained BIR for identity checks,
diagnostics, and emission context. It must not rederive call-plan decisions
already present in `PreparedCallPlan` or its argument/effect records.

## Latest Closure Review Finding

The Step 5 closure review after the aggregate-address broader backend
checkpoint rejected closure. The aggregate-address publication gate now consumes
`PreparedCallArgumentPlan::allows_local_aggregate_address_publication`, and the
scan did not find retained `arg_abi` or `arg_types` decision reads in
`calls_dispatch_bridge.cpp` for that route.

The source idea remains open because `calls_argument_sources.cpp` still decides
local-frame address publication eligibility by reading retained call metadata:

- `call_argument_is_pointer` reads `CallInst::arg_types` and falls back to
  `CallInst::args` to decide whether an argument is pointer-like.
- `call_argument_is_byval_copy` reads `CallInst::arg_abi` to exclude byval
  copies.
- `call_argument_allows_local_frame_address_publication` combines those
  retained reads with a callee spelling check before
  `make_local_frame_address_call_argument_source` can publish a local-frame
  address.

Close-time regression guard generation was not needed because closure was
rejected before the close gate.

## Read First

- `ideas/open/02_aarch64_calls_emission_consolidation.md`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls_argument_sources.cpp`
- `src/backend/mir/aarch64/codegen/calls_dispatch_bridge.cpp`
- `src/backend/mir/aarch64/codegen/calls_dispatch_bridge.hpp`
- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- Focused backend local-frame, publication, byval, and prepared-call tests
  under `tests/backend/mir/`

## Current Targets / Scope

- Retained `bir::CallInst::arg_types` reads in
  `call_argument_is_pointer`.
- Retained `bir::CallInst::arg_abi` reads in
  `call_argument_is_byval_copy`.
- The local-frame publication gate in
  `call_argument_allows_local_frame_address_publication`.
- Helper signatures that pass `instruction_index` or `CallInst`-derived
  context only to reconstruct publication eligibility.
- Focused tests that prove prepared facts own the selected local-frame
  publication decision.

## Non-Goals

- Do not invent a new shared call-plan API unless the selected blocker proves
  a required prepared fact is missing.
- Do not perform broad dispatch cleanup or work from
  `ideas/open/03_dispatch_responsibility_reduction.md`.
- Do not change behavior solely to reduce line count.
- Do not weaken unsupported or expected-output contracts.
- Do not move AArch64 emission details into the shared planner.
- Do not revisit aggregate-address or byval aggregate lane lowering unless this
  checkpoint exposes a concrete retained-authority dependency there.

## Working Model

- Prefer a prepared fact already present on `PreparedCallArgumentPlan`,
  `PreparedMoveResolution`, or `PreparedCallBoundaryEffectPlan`.
- If local-frame publication needs a fact that is genuinely missing, stop and
  record the missing prepared authority in `todo.md` instead of rebuilding the
  decision locally.
- A retained `bir::CallInst` read is acceptable only when it validates that the
  prepared plan matches the instruction being emitted or reports a diagnostic
  for inconsistent prepared data.
- A surviving helper boundary is acceptable only when its parameters describe
  emission context, not call-planning authority.

## Execution Rules

- Start with the exact retained metadata read being removed.
- Delete local reconstruction before adding any new helper.
- Keep each code slice narrow enough for a fresh build plus focused backend
  proof.
- Escalate to `^backend_` after changing publication eligibility,
  call-boundary effect ordering, or shared prepared-call behavior.
- Reject helper renames, expectation rewrites, and testcase-shaped shortcuts as
  progress.

## Step 1: Select One Local-Frame Publication Authority Leak

Goal: choose the next duplicate-authority local-frame publication path to
remove and prove whether the needed prepared fact already exists.

Primary targets:

- `call_argument_is_pointer` in `calls_argument_sources.cpp`
- `call_argument_is_byval_copy` in `calls_argument_sources.cpp`
- `call_argument_allows_local_frame_address_publication` in
  `calls_argument_sources.cpp`
- Any helper declaration that survives only to pass retained call metadata into
  that gate

Actions:

- Identify the exact retained `CallInst::arg_abi` or `CallInst::arg_types`
  read and the publication decision it makes.
- Map that decision to existing prepared argument, move, or boundary-effect
  fields.
- Select one coherent local-frame publication target for the next executor
  packet.
- Record the selected target, expected deletion path, and proof command in
  `todo.md`.

Completion check:

- `todo.md` names one local-frame publication authority leak, the prepared
  replacement fact or missing prepared-fact blocker, and the focused proof
  scope.

## Step 2: Remove The Selected Local-Frame Publication Decision

Goal: replace the selected retained metadata publication decision with
prepared-fact consumption or stop with a precise missing-prepared-fact blocker.

Actions:

- Delete the retained `CallInst::arg_abi` or `CallInst::arg_types` decision
  from the selected local-frame publication path.
- Keep retained BIR checks only for instruction identity validation,
  callee-specific diagnostics, or emission context.
- Tighten helper signatures so obsolete `CallInst` or `instruction_index`
  parameters do not remain when prepared facts are sufficient.
- Update focused tests only to preserve or strengthen the prepared-authority
  contract.
- Run `cmake --build --preset default` plus the focused backend proof selected
  in Step 1.

Completion check:

- The selected local-frame publication path no longer owns call-planning
  authority locally, and `test_after.log` records passing build plus focused
  proof.

## Step 3: Consolidate The Local-Frame Helper Boundary

Goal: remove the helper/API boundary made obsolete by Step 2 or document why
the surviving boundary is emission-only.

Actions:

- Remove obsolete declarations from `calls.hpp` and helper-local prototypes
  when local-frame publication no longer needs retained metadata context.
- Merge helper code back into a surviving owner when the helper no longer
  describes a real emission-only boundary.
- Update build metadata only if a translation unit is retired.
- Keep edits limited to local-frame call-argument source publication
  ownership.
- Run a fresh build and the focused backend proof after each coherent helper
  boundary change.

Completion check:

- The affected helper boundary is retired or explicitly emission-only, with
  build metadata and include graphs matching the new boundary.

## Step 4: Broader Backend Checkpoint

Goal: prove the latest local-frame publication-authority checkpoint did not
regress adjacent call, byval, aggregate, or printer behavior.

Actions:

- Run the supervisor-selected broader backend validation scope.
- Include focused AArch64 local-frame, publication, byval, aggregate-address,
  prepared-call boundary, and any affected x86 shared-boundary tests when
  shared prepared-call behavior was touched.
- Record exact proof commands and results in `todo.md`.

Completion check:

- The broader checkpoint passes, or `todo.md` records a precise blocker tied
  to the active source idea.

## Step 5: Closure Review

Goal: decide whether the source idea is complete after this checkpoint or
whether another runbook checkpoint is needed.

Actions:

- Recheck all surviving `calls*.cpp`/`calls*.hpp` retained `CallInst` ABI/type
  reads against the source idea acceptance criteria.
- Confirm target-local calls code no longer rederives decisions already
  present in shared prepared facts.
- Confirm surviving helper files have explicit emission-only ownership.
- If durable remaining work exists, keep the idea open and request another
  runbook checkpoint instead of closing.

Completion check:

- Lifecycle state gives the supervisor an unambiguous close/keep-active
  decision.
