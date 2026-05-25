# AArch64 Calls Emission Consolidation Residual Authority Checkpoint

Status: Active
Source Idea: ideas/open/02_aarch64_calls_emission_consolidation.md

## Purpose

Continue the AArch64 call-emission consolidation after the Step 5 closure
review rejected closure for the register byval size authority-removal
checkpoint.

## Goal

Remove the remaining target-local uses of retained call ABI/type metadata that
still decide call-boundary argument placement, aggregate address publication
eligibility, or byval shape when prepared call-plan facts should own those
decisions.

## Core Rule

Target-local AArch64 calls code may inspect retained BIR for identity checks,
diagnostics, and emission context. It must not rederive call-plan decisions
already present in `PreparedCallPlan` or its argument/effect records.

## Latest Closure Review Finding

The broader backend checkpoint after register byval size authority removal
recorded `^backend_` passing 162/162 in `test_before.log`, and the regression
guard accepted that canonical artifact for the unchanged lifecycle review
state. The source idea is not complete. The closure review confirmed that
target-local calls code still has retained `CallInst::arg_abi` or
`CallInst::arg_types` decision reads that decide publication and byval shape
where prepared call-plan facts should own the decision:

- `calls_dispatch_bridge.cpp` still decides local aggregate address
  publication eligibility from retained `CallInst::arg_abi` and
  `CallInst::arg_types`.
- `calls_argument_sources.cpp` still decides call argument pointer/byval local
  frame address publication from retained `CallInst::arg_types` and
  `CallInst::arg_abi`.
- `calls_byval_aggregates.cpp` still rechecks retained `CallInst::arg_abi`
  shape in `aarch64_indirect_byval_argument_size_bytes`,
  `aarch64_stack_byval_argument_size_bytes`, and
  `aarch64_indirect_register_byval_argument`.
- `calls_dispatch_bridge.hpp` still exposes `CallInst`-shaped helper
  boundaries that need to be retired or justified as emission-only once the
  publication path no longer reconstructs call-plan decisions.

## Read First

- `ideas/open/02_aarch64_calls_emission_consolidation.md`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls_common.cpp`
- `src/backend/mir/aarch64/codegen/calls_argument_sources.cpp`
- `src/backend/mir/aarch64/codegen/calls_dispatch_bridge.cpp`
- `src/backend/mir/aarch64/codegen/calls_byval_aggregates.cpp`
- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `src/backend/mir/aarch64/codegen/calls_preservation.cpp`
- Focused backend call tests under `tests/backend/mir/`

## Current Targets / Scope

- Retained `bir::CallInst::arg_abi` and `arg_types` reads inside
  `calls*.cpp` that decide aggregate publication eligibility or byval lane
  size.
- Prepared argument facts that can replace those reads.
- Helper declarations in `calls.hpp` that expose obsolete ABI-reconstruction
  boundaries.
- Focused tests that prove prepared facts own the selected call-boundary
  decision.

## Non-Goals

- Do not invent a new shared call-plan API unless the selected blocker proves
  a required prepared fact is missing.
- Do not perform broad dispatch cleanup or work from
  `ideas/open/03_dispatch_responsibility_reduction.md`.
- Do not change behavior solely to reduce line count.
- Do not weaken unsupported or expected-output contracts.
- Do not move AArch64 emission details into the shared planner.

## Working Model

- Prefer a prepared fact already present on `PreparedCallArgumentPlan`,
  `PreparedMoveResolution`, or `PreparedCallBoundaryEffectPlan`.
- If a fact is genuinely missing, stop and record the missing prepared
  authority in `todo.md` instead of rebuilding the decision locally.
- A retained `bir::CallInst` read is acceptable only when it validates that the
  prepared plan matches the instruction being emitted or reports a diagnostic
  for inconsistent prepared data.

## Execution Rules

- Start with the exact retained metadata read being removed.
- Delete local reconstruction before adding any new helper.
- Keep each code slice narrow enough for a fresh build plus focused backend
  proof.
- Escalate to `^backend_` after changing byval aggregate lane lowering,
  publication eligibility, or call-boundary effect ordering.
- Reject helper renames, expectation rewrites, and testcase-shaped shortcuts as
  progress.

## Step 1: Select One Retained Metadata Authority Leak

Goal: choose the next duplicate-authority path to remove and prove whether the
needed prepared fact already exists.

Primary targets:

- `call_argument_allows_local_aggregate_address_publication` in
  `calls_dispatch_bridge.cpp`
- `call_argument_allows_local_frame_address_publication` and its
  pointer/byval helpers in `calls_argument_sources.cpp`
- byval size and indirect-register predicates in `calls_byval_aggregates.cpp`

Actions:

- For each target, identify the exact retained `CallInst::arg_abi` or
  `CallInst::arg_types` read and the decision it makes.
- Map that decision to existing prepared argument, move, or boundary-effect
  fields.
- Select one coherent target for the next executor packet.
- Record the selected target, expected deletion path, and proof command in
  `todo.md`.

Completion check:

- `todo.md` names one authority leak, the prepared replacement fact or missing
  prepared-fact blocker, and the focused proof scope.

## Step 2: Remove The Selected Local Decision

Goal: replace the selected retained metadata decision with prepared-fact
consumption or stop with a precise missing-prepared-fact blocker.

Actions:

- Delete the retained `CallInst::arg_abi` or `CallInst::arg_types` decision
  from the selected path.
- Keep retained BIR checks only for instruction identity validation or
  diagnostics.
- Tighten helper signatures so obsolete `CallInst` parameters do not remain
  when prepared facts are sufficient.
- Update focused tests only to preserve or strengthen the prepared-authority
  contract.
- Run `cmake --build --preset default` plus the focused backend proof selected
  in Step 1.

Completion check:

- The selected path no longer owns call-planning authority locally, and
  `test_after.log` records passing build plus focused proof.

## Step 3: Consolidate The Affected Helper Boundary

Goal: remove the helper/API boundary made obsolete by Step 2 or document why
the surviving boundary is emission-only.

Actions:

- Remove obsolete declarations from `calls.hpp`.
- Merge helper code back into a surviving owner when the helper no longer
  describes a real emission-only boundary.
- Update build metadata only if a translation unit is retired.
- Keep `calls_dispatch_bridge.*` edits limited to call-boundary ownership.
- Run a fresh build and the focused backend proof after each coherent helper
  boundary change.

Completion check:

- The affected helper boundary is retired or explicitly emission-only, with
  build metadata and include graphs matching the new boundary.

## Step 4: Broader Backend Checkpoint

Goal: prove the latest authority-removal checkpoint did not regress adjacent
call or printer behavior.

Actions:

- Run the supervisor-selected broader backend validation scope.
- Include focused AArch64 call-boundary, prepared call boundary, byval, and
  any affected x86 shared-boundary tests when shared prepared-call behavior was
  touched.
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
- Confirm target-local calls code no longer rederives decisions already present
  in shared prepared facts.
- Confirm surviving helper files have explicit emission-only ownership.
- If durable remaining work exists, keep the idea open and request another
  runbook checkpoint instead of closing.

Completion check:

- Lifecycle state gives the supervisor an unambiguous close/keep-active
  decision.
