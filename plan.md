# AArch64 Calls Prepared Authority Repair Runbook

Status: Active
Source Idea: ideas/open/52_aarch64_calls_prepared_authority_repair.md
Activated after: ideas/closed/57_aarch64_variadic_va_arg_register_save_progression_repair.md

## Purpose

Repair remaining duplicate call authority in
`src/backend/mir/aarch64/codegen/calls.cpp` by making call lowering consume
prepared call plans, argument/result plans, source selections, boundary
effects, move bundles, preservation facts, and value homes instead of
recovering those facts locally.

## Goal

Remove local call-authority recovery from `calls.cpp` behind shared prepared
facts or narrowly added shared queries, while preserving AAPCS64 call spelling
and existing supported behavior.

## Core Rule

Call lowering must not recover semantic ABI bindings, argument producers,
callee sources, call-boundary sources, or result registers from raw operands,
prepared-name spelling, same-block producer walks, or ad hoc move-bundle scans
when prepared authority can carry the fact.

## Read First

- `ideas/open/52_aarch64_calls_prepared_authority_repair.md`
- `todo.md`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- prepared call and move planning surfaces:
  - `src/backend/prealloc/call_plans.cpp`
  - `src/backend/prealloc/call_plans.hpp`
  - `src/backend/prealloc/prepared_lookups.cpp`
  - `src/backend/prealloc/prepared_lookups.hpp`
  - `src/backend/prealloc/regalloc/consumer_moves.cpp`
  - `src/backend/prealloc/regalloc/consumer_moves.hpp`
- parked blocker context:
  - `ideas/closed/57_aarch64_variadic_va_arg_register_save_progression_repair.md`
  - `review/calls-step3-byval-lane-review.md`

## Current Targets

- `lower_before_call_immediate_binding`
- `find_prepared_frame_slot_call_argument_move`
- `materialize_scalar_call_argument_value`
- `lower_scalar_call_argument_producers`
- `materialize_direct_global_select_chain_call_argument`
- `materialize_call_boundary_source_to_destination`
- `find_bir_value_for_prepared_name`
- `materialize_indirect_call_callee_to_prepared_register`
- `resolve_indirect_callee_local_load_store`
- `emit_indirect_callee_value_to_register_with_csel`
- `record_call_result_source_register`

## Non-Goals

- Do not change AAPCS64 register/stack staging, direct `bl`, indirect `blr`,
  stack cleanup, source reload sequencing, or ABI result-store instruction
  spelling except to consume prepared semantic facts.
- Do not reopen aggregate `va_arg` register-save progression or treat
  `variadic.cpp` as part of this route.
- Do not fold ALU, dispatch value-materialization, comparison, memory, or edge
  preservation repairs into this idea.
- Do not add named-case logic for `00204`, `myprintf`, one function name, one
  temporary, or one string literal.
- Do not weaken expectations or mark supported tests unsupported.

## Working Model

- Earlier idea 52 work established the direct variadic by-value aggregate
  single-GPR lane facts needed by caller-side call publication.
- Idea 57 closed the callee-side aggregate `va_arg` register-save progression
  blocker that kept focused proof red after that caller-side repair.
- This activation resumes idea 52 for its broader duplicate call-authority
  acceptance criteria; it should not redo the variadic route or claim
  variadic-only proof as call-authority completion.
- Some local helper scans in `calls.cpp` may remain valid as target-local
  register or emission mechanics. Reject only the parts that select semantic
  source, binding, movement, callee, or result facts locally.

## Execution Rules

- Start each packet by naming the duplicate authority being removed and the
  prepared fact or query that will replace it.
- Prefer consuming existing `PreparedCallPlan`, `PreparedCallArgumentPlan`,
  `PreparedCallArgumentSourceSelection`, `PreparedCallBoundaryEffectPlan`,
  `PreparedMoveBundle`, `PreparedIndirectCalleePlan`,
  `PreparedMemoryReturnPlan`, and `PreparedCallResultPlan` facts before adding
  a new query.
- Add a shared query only when current prepared records cannot answer a real
  non-testcase-shaped consumer contract.
- Keep implementation slices narrow enough for focused build and backend test
  proof, then escalate validation when a query affects multiple call routes.
- Preserve reviewer reject signals from the source idea: no expectation
  downgrades, helper-renames-as-progress, deeper same-block walks, or
  broad rewrites outside call authority.

## Steps

### Step 1: Rebaseline remaining call-authority gaps

Goal: identify which idea 52 fallback paths remain after the closed variadic
register-save route and choose the first bounded executor packet.

Primary target: `src/backend/mir/aarch64/codegen/calls.cpp`

Actions:

- Inspect the current implementations of the target helpers listed above.
- Classify each remaining fallback as one of: already prepared-authority
  consumer, duplicate semantic recovery needing replacement, target-local
  emission mechanic, or out-of-scope adjacent owner.
- Confirm that direct variadic by-value aggregate lane publication is not the
  active blocker anymore.
- Record the first bounded packet and its proof command in `todo.md`.

Completion check:

- `todo.md` records the chosen first call-authority packet, the helper(s) it
  owns, and the focused validation subset the supervisor should delegate.

### Step 2: Repair immediate ABI binding and frame-slot argument move authority

Goal: replace argument-specific ABI binding and move-bundle scans with shared
prepared argument or move lookup authority.

Primary target: `src/backend/mir/aarch64/codegen/calls.cpp`

Actions:

- Replace `lower_before_call_immediate_binding` scans over
  `call_plan.arguments` with an existing or new prepared lookup keyed by the
  call and ABI binding.
- Replace `find_prepared_frame_slot_call_argument_move` reclassification scans
  with an existing or new prepared move lookup keyed by call argument plan or
  argument ABI index.
- Keep call staging and move emission spelling unchanged.
- Prove both immediate-only and frame-slot call argument routes.

Completion check:

- Immediate binding and frame-slot argument move selection no longer depend on
  local scans for semantic matching, and focused call tests pass.

### Step 3: Repair scalar call-argument producer materialization authority

Goal: make scalar call-argument materialization consume prepared producer or
source-selection facts instead of recursive same-block producer recovery.

Primary target: `src/backend/mir/aarch64/codegen/calls.cpp`

Actions:

- Replace same-block scalar producer recursion in
  `materialize_scalar_call_argument_value` and
  `lower_scalar_call_argument_producers` with an existing or new prepared
  call-argument producer/materialization query.
- Ensure direct-global select-chain call arguments consume the shared
  select-chain or producer authority selected by prepared planning.
- Do not move this repair into ALU or dispatch value-materialization without a
  shared-query contract.

Completion check:

- Scalar call arguments and direct-global select-chain arguments materialize
  from prepared authority, not deeper producer walks, with same-feature proof.

### Step 4: Repair call-boundary source and indirect-callee authority

Goal: remove prepared-name BIR scans and local indirect-callee source recovery
from call-boundary materialization.

Primary target: `src/backend/mir/aarch64/codegen/calls.cpp`

Actions:

- Replace `find_bir_value_for_prepared_name` use in
  `materialize_call_boundary_source_to_destination` with a prepared boundary
  source payload or source-producer query.
- Replace indirect callee local-load, select-chain, and CSEL source recovery
  with prepared indirect-callee source/materialization facts.
- Keep direct `blr` spelling and scratch sequencing unchanged unless prepared
  consumption requires a mechanical adjustment.

Completion check:

- Boundary source and indirect-callee materialization no longer depend on BIR
  result spelling, call operand spelling, or local same-block recovery.

### Step 5: Repair after-call result publication authority

Goal: make call result publication consume prepared result or after-call move
lookup facts rather than raw after-call move-bundle scans.

Primary target: `src/backend/mir/aarch64/codegen/calls.cpp`

Actions:

- Audit `record_call_result_source_register` and related result publication
  paths against `PreparedCallResultPlan`.
- Add or consume a shared after-call result lookup by destination value id and
  register bank when `PreparedCallResultPlan` alone is insufficient.
- Preserve ABI result register and memory-return behavior.

Completion check:

- Call result publication uses prepared result authority and focused result
  publication tests pass.

### Step 6: Validate call-authority acceptance and decide closure readiness

Goal: decide whether idea 52 is complete or needs another bounded call packet.

Primary target: `todo.md`, `test_before.log`, and `test_after.log`

Actions:

- Compare the accepted implementation against every source-idea acceptance
  criterion.
- Run the supervisor-selected focused proof and any broader backend or full
  CTest validation required by the blast radius.
- Record any remaining duplicate call-authority fallback as a precise next
  packet, not a vague follow-up.
- Reject routes that prove only one named testcase, downgrade expectations, or
  leave the same local authority behind a renamed helper.

Completion check:

- `todo.md` records either closure-ready acceptance evidence for idea 52 or
  the next bounded call-authority packet.
