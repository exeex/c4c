# AArch64 Special Carrier Prepared Policy Cleanup Runbook

Status: Active
Source Idea: ideas/open/72_aarch64_special_carrier_prepared_policy_cleanup.md

## Purpose

Make AArch64 i128 and f128 lowering consume prepared carrier, helper, ABI,
lane, resource, and clobber policy facts before emitting target-local records.

## Goal

Replace duplicated i128/f128 carrier and helper-policy decisions in AArch64
lowering with prepared authority consumption while preserving AArch64 pair,
lane, vector, memory-address, transport, and helper-call record emission
locally.

## Core Rule

Prepared special-carrier facts decide carrier selection, runtime-helper policy,
lane bindings, ABI transitions, preservation, resource ownership, scalar
result ownership, memory-backed carrier facts, and clobbers; AArch64 code
decides only target instruction forms, register views, address spelling,
helper-boundary records, and emitted machine records.

## Read First

- `ideas/open/72_aarch64_special_carrier_prepared_policy_cleanup.md`
- `src/backend/mir/aarch64/codegen/i128_ops.cpp`
- `src/backend/mir/aarch64/codegen/f128.cpp`
- `src/backend/prealloc/i128_runtime_helpers.*`
- `src/backend/prealloc/f128_runtime_helpers.*`
- `src/backend/prealloc/special_carriers.*`
- prepared value-home, addressing, and regalloc lookup surfaces used by these owners

## Current Targets

- AArch64 i128 lowering that still locally decides carrier selection,
  runtime-helper policy, lane bindings, ABI policy, call preservation,
  clobbers, or value-home/regalloc facts before pair/lane emission.
- AArch64 f128 lowering that still locally decides carrier selection,
  runtime-helper policy, ABI transition/result ownership, resource ownership,
  clobbers, scalar result ownership, memory-backed carriers, addressing, or
  value homes before vector/helper emission.
- Shared prepared-authority call sites only where existing prepared facts need
  to be consumed more directly.

## Non-Goals

- Do not perform variadic entry-plan cleanup; that belongs to
  `ideas/open/73_aarch64_variadic_prepared_entry_plan_cleanup.md`.
- Do not perform aggregate transport planning; that belongs to
  `ideas/open/75_shared_aggregate_transport_plan_probe.md`.
- Do not move AArch64 pair/lane instruction selection, vector instruction
  selection, memory-address spelling, or helper-boundary record construction
  into shared prealloc code.
- Do not rework helper ABIs beyond consuming already prepared policy.
- Do not mix unrelated ALU, memory, call, dispatch, or variadic cleanup into
  this route.

## Working Model

- `PreparedI128Carrier*`, prepared i128 runtime-helper facts, lane bindings,
  ABI policy, call-preservation policy, clobber policy, value homes, and
  regalloc facts are the authority for i128 transport and helper policy.
- `PreparedF128Carrier*`, prepared f128 runtime-helper facts, ABI
  transition/result ownership, resource and clobber policy, scalar result
  ownership, memory-backed carrier facts, addressing, and value homes are the
  authority for f128 carrier and helper policy.
- AArch64 may still choose pair/lane instruction records, shift/count
  admissibility, compare sequence emission, Q/vector register conversion,
  f128 memory address spelling, helper-boundary records, and final
  transport/helper emission locally.

## Execution Rules

- Work in narrow, reviewable steps tied to one special-carrier authority
  boundary at a time.
- Prefer consuming prepared facts over adding AArch64-local policy wrappers.
- When replacing a local decision, leave code or test evidence that the
  prepared lookup is now the source of truth.
- If a needed prepared fact is missing or ambiguous, stop and record the
  blocker in `todo.md` instead of adding a target-local substitute.
- Each code-changing step needs fresh build or compile proof; broader CTest or
  regression-guard proof is required when a packet changes shared prepared
  authority or both i128 and f128 owners.

## Ordered Steps

### Step 1: Map Special Carrier Policy Duplication

Goal: identify the exact AArch64 i128 and f128 decisions that duplicate
prepared carrier, helper, ABI, lane, resource, value-home, or clobber authority.

Primary targets:

- `src/backend/mir/aarch64/codegen/i128_ops.cpp`
- `src/backend/mir/aarch64/codegen/f128.cpp`
- `src/backend/prealloc/i128_runtime_helpers.*`
- `src/backend/prealloc/f128_runtime_helpers.*`
- `src/backend/prealloc/special_carriers.*`

Actions:

- Inspect i128 pair/lane and helper emission paths for carrier selection,
  runtime-helper policy, lane binding, ABI, preservation, clobber, value-home,
  and regalloc decisions.
- Inspect f128 vector/helper emission paths for carrier selection,
  runtime-helper policy, ABI transition/result ownership, resource ownership,
  clobbers, scalar result ownership, memory-backed carrier facts, addressing,
  and value-home decisions.
- Cross-reference each local decision with existing prepared special-carrier,
  runtime-helper, value-home, addressing, and regalloc lookup surfaces.
- Classify each decision as prepared authority consumption, missing prepared
  authority, or target-local AArch64 emission policy.
- Record the first executable implementation packet in `todo.md`, including
  the proof command the supervisor delegates.

Completion check:

- `todo.md` names the first concrete implementation packet and distinguishes
  prepared special-carrier authority from AArch64-local pair, lane, vector,
  address, helper-boundary, and machine-record emission policy.

### Step 2: Consume Prepared I128 Carrier And Lane Facts

Goal: make i128 lowering consume prepared carrier and lane policy before
target-local pair/lane instruction emission.

Primary target: `src/backend/mir/aarch64/codegen/i128_ops.cpp`

Actions:

- Replace local i128 carrier selection and lane-binding reconstruction with
  consumption of `PreparedI128Carrier*` and prepared lane facts where those
  facts already exist.
- Keep AArch64 pair/lane instruction selection, shift/count admissibility,
  compare sequence emission, register views, and machine-record construction
  local.
- Add or update focused proof for i128 carrier and lane cases touched by the
  change.

Completion check:

- I128 lowering consumes prepared carrier and lane facts before pair/lane
  emission, and the focused AArch64 i128 proof is green.

### Step 3: Consume Prepared I128 Runtime Helper And ABI Policy

Goal: make i128 helper lowering consume prepared runtime-helper, ABI,
call-preservation, clobber, value-home, and regalloc facts.

Primary targets:

- `src/backend/mir/aarch64/codegen/i128_ops.cpp`
- `src/backend/prealloc/i128_runtime_helpers.*`

Actions:

- Replace local runtime-helper policy, helper ABI, preservation, clobber,
  value-home, and regalloc reconstruction with prepared i128 helper facts
  where those facts already exist.
- Preserve AArch64 helper-boundary record construction and final transport
  emission locally.
- Prove helper-boundary cases that would expose accidental local policy
  reconstruction.

Completion check:

- I128 helper emission treats prepared helper and ABI policy as authoritative,
  with focused helper proof green.

### Step 4: Consume Prepared F128 Carrier And Memory-Backed Facts

Goal: make f128 lowering consume prepared carrier, memory-backed carrier,
addressing, and value-home facts before target-local vector or memory emission.

Primary target: `src/backend/mir/aarch64/codegen/f128.cpp`

Actions:

- Replace local f128 carrier, memory-backed carrier, addressing, and value-home
  reconstruction with `PreparedF128Carrier*` and related prepared lookup facts
  where those facts already exist.
- Keep Q/vector register conversion, memory address spelling, load/store
  instruction choice, and machine-record construction local.
- Add or update focused proof for f128 carrier and memory-backed cases touched
  by the change.

Completion check:

- F128 lowering consumes prepared carrier and memory-backed facts before
  target emission, and focused f128 carrier proof is green.

### Step 5: Consume Prepared F128 Runtime Helper And Result Policy

Goal: make f128 helper lowering consume prepared runtime-helper, ABI
transition/result ownership, resource, clobber, and scalar result facts.

Primary targets:

- `src/backend/mir/aarch64/codegen/f128.cpp`
- `src/backend/prealloc/f128_runtime_helpers.*`

Actions:

- Replace local f128 runtime-helper policy, ABI transition/result ownership,
  resource ownership, clobber policy, and scalar result ownership
  reconstruction with prepared f128 helper facts where those facts already
  exist.
- Preserve AArch64 helper-boundary records, Q/vector conversion, scalar result
  rendering, and final transport/helper emission locally.
- Prove f128 helper-boundary and result ownership cases that would expose
  duplicated authority.

Completion check:

- F128 helper emission treats prepared helper, ABI/result, resource, and
  clobber facts as authoritative, with focused helper proof green.

### Step 6: Acceptance Validation And Drift Check

Goal: verify the completed route satisfies the source idea without overfitting
or moving target-local policy into shared authority.

Primary targets:

- `plan.md`
- `todo.md`
- implementation files touched by Steps 2 through 5
- `ideas/open/72_aarch64_special_carrier_prepared_policy_cleanup.md`

Actions:

- Review the final diff against the source idea's in-scope, out-of-scope, and
  reviewer reject signals.
- Confirm no expectations were weakened, no supported paths were marked
  unsupported, and no named testcase shortcut was added.
- Confirm AArch64 pair/lane instruction selection, vector instruction
  selection, shift/count admissibility, compare sequence emission, memory
  address spelling, helper-boundary records, and final machine-record emission
  remain target-local.
- Run the supervisor-selected acceptance proof, escalating to regression guard
  or broader CTest when shared prepared authority or both special-carrier
  owners were changed.
- Record final proof and any remaining blockers in `todo.md`.

Completion check:

- `todo.md` records acceptance proof and drift-check results, and the
  supervisor can decide whether idea 72 is ready for lifecycle close or another
  runbook checkpoint.
