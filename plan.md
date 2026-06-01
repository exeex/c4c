# AArch64 Scalar And Control-Flow Prepared Authority Cleanup Runbook

Status: Active
Source Idea: ideas/open/71_aarch64_scalar_control_flow_prepared_authority_cleanup.md

## Purpose

Make AArch64 scalar, cast, comparison, and branch lowering consume BIR semantic
facts and prepared scalar/control-flow authority before choosing target-local
AArch64 records.

## Goal

Replace duplicated semantic decisions in the AArch64 scalar and control-flow
path with prepared lookups while preserving AArch64 opcode selection,
immediate legality, materialization, condition-code spelling, and record
construction locally.

## Core Rule

BIR and prepared facts decide scalar meaning, producer/home provenance,
computed-value availability, branch-condition authority, short-circuit shape,
compare joins, fused compare operands, and branch labels; AArch64 code decides
only target instruction forms, encodability, materialization, condition-code
spelling, and emitted machine records.

## Read First

- `ideas/open/71_aarch64_scalar_control_flow_prepared_authority_cleanup.md`
- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/cast_ops.cpp`
- `src/backend/mir/aarch64/codegen/comparison.cpp`
- `src/backend/prealloc/control_flow.hpp`
- `src/backend/prealloc/prepared_lookups.*`
- BIR opcode and type query surfaces used by these owners

## Current Targets

- AArch64 ALU helpers that still re-derive BIR binary opcode/type facts,
  immediate-binary classification, prepared scalar producer homes, or
  computed-value availability.
- AArch64 cast helpers that still duplicate BIR cast opcode/type authority
  before selecting AArch64 extension/conversion records.
- AArch64 comparison and branch helpers that still re-decide branch conditions,
  short-circuit plans, compare joins, fused compare operands, or branch labels.
- Shared prepared authority call sites only where existing facts need to be
  consumed more directly.

## Non-Goals

- Do not perform a broad line-count rewrite of ALU, cast, or comparison owners.
- Do not move AArch64 immediate admissibility, condition-code spelling, or
  machine-record construction into BIR or prealloc.
- Do not fold repeated local register-view helpers; that belongs to
  `ideas/open/74_aarch64_local_scalar_register_helper_fold_back.md`.
- Do not change BIR semantics or weaken tests to fit one target testcase.
- Do not mix memory, call, special-carrier, variadic, or printer cleanup into
  this route.

## Working Model

- `bir::BinaryOpcode`, `bir::CastOpcode`, `bir::TypeKind`, and BIR value/type
  queries are the source of scalar and cast semantics.
- `PreparedValueHomeLookups`, `PreparedValueLocationFunction`, storage plans,
  regalloc facts, decoded home storage, `PreparedSameBlockScalarProducer`,
  `PreparedComputedValue`, and prepared immediate-binary classification describe
  scalar producer and placement authority before target instruction selection.
- `PreparedControlFlowFunction`, `PreparedBranchCondition`,
  `PreparedShortCircuitBranchPlan`, `PreparedMaterializedCompareJoin*`,
  `PreparedFusedCompareOperandProducer`, and prepared branch labels describe
  control-flow and compare authority before AArch64 emits compare and branch
  records.
- AArch64 may still choose immediate vs materialized forms, scalar extension
  instructions, compare operand materialization, condition-code spelling, and
  final branch or scalar records locally.

## Execution Rules

- Work in narrow, reviewable steps tied to one authority boundary at a time.
- Prefer semantic prepared-fact consumption over testcase-shaped matching.
- When replacing a local derivation, leave code or test evidence that the
  prepared lookup is now the source of truth.
- If a needed prepared fact is missing or ambiguous, stop and record the
  blocker in `todo.md` instead of adding a target-local substitute.
- Each code-changing step needs fresh build or compile proof; broader CTest or
  regression-guard proof is required when a packet changes shared prepared
  lookup behavior or multiple AArch64 owners.

## Ordered Steps

### Step 1: Map Scalar And Control-Flow Authority Duplication

Goal: identify the exact AArch64 scalar, cast, comparison, and branch decisions
that duplicate BIR or prepared authority.

Primary targets:

- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/cast_ops.cpp`
- `src/backend/mir/aarch64/codegen/comparison.cpp`

Actions:

- Inspect helper entry points that select scalar opcodes, cast records,
  comparison records, branch conditions, short-circuit branches, compare joins,
  fused compare operands, or branch labels.
- Cross-reference each local decision with available BIR opcode/type queries
  and prepared lookup surfaces.
- Classify each duplicated decision as BIR scalar/cast semantics,
  scalar producer/home placement, immediate-binary classification,
  branch-condition authority, short-circuit planning, compare-join authority,
  fused-operand provenance, branch-label authority, or target-local AArch64
  instruction policy.
- Record the first executable implementation packet in `todo.md`, including
  the proof command the supervisor delegates.

Completion check:

- `todo.md` names the first concrete implementation packet and distinguishes
  shared authority consumption from AArch64-local opcode, immediate,
  condition-code, and record policy.

### Step 2: Consume BIR Scalar And Cast Semantic Facts

Goal: make ALU and cast lowering use BIR opcode/type facts before target-local
record selection.

Primary targets:

- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/cast_ops.cpp`

Actions:

- Replace local binary opcode, cast opcode, and scalar type reconstruction with
  `bir::BinaryOpcode`, `bir::CastOpcode`, `bir::TypeKind`, and BIR value/type
  query consumption where those facts already exist.
- Preserve AArch64 immediate admissibility, fallback materialization, scalar
  extension instruction choice, and final record construction locally.
- Add or update focused proof for scalar binary and cast cases touched by the
  change.

Completion check:

- ALU and cast lowering consult BIR semantic facts before AArch64 instruction
  selection, and the focused AArch64 scalar/cast proof is green.

### Step 3: Consume Prepared Scalar Producer And Placement Facts

Goal: make scalar lowering consume prepared producer, home, storage, regalloc,
decoded-home, computed-value, and immediate-binary classification facts.

Primary target: `src/backend/mir/aarch64/codegen/alu.cpp`

Actions:

- Replace local scalar producer/home reconstruction with
  `PreparedValueHomeLookups`, `PreparedValueLocationFunction`, storage plans,
  regalloc facts, decoded home storage, `PreparedSameBlockScalarProducer`, and
  `PreparedComputedValue` where those facts already exist.
- Consume prepared immediate-binary classification before choosing AArch64
  immediate or materialized operand forms.
- Keep target-local register views, temporary materialization, immediate
  encoding checks, opcode selection, and record construction in AArch64 code.
- Prove same-block producer, computed-value, and immediate-binary cases that
  would expose accidental re-derivation.

Completion check:

- Scalar lowering treats prepared producer/placement facts as authoritative and
  the relevant AArch64 scalar subset remains green.

### Step 4: Consume Prepared Branch And Short-Circuit Facts

Goal: make branch lowering consume prepared control-flow, branch-condition,
short-circuit, and label facts before target-local branch record emission.

Primary target: `src/backend/mir/aarch64/codegen/comparison.cpp`

Actions:

- Replace local branch-condition and short-circuit policy reconstruction with
  `PreparedControlFlowFunction`, `PreparedBranchCondition`,
  `PreparedShortCircuitBranchPlan`, and prepared branch labels where those
  facts already exist.
- Preserve AArch64 condition-code spelling, compare instruction selection,
  branch record construction, and final label rendering locally.
- Add or update focused proof for direct branches and short-circuit branches
  touched by the change.

Completion check:

- Branch lowering consumes prepared control-flow facts before emitting AArch64
  compare/branch records, and focused branch proof is green.

### Step 5: Consume Prepared Compare Join And Fused Operand Facts

Goal: make comparison lowering consume prepared materialized-compare join and
fused compare operand producer facts before operand materialization and compare
record construction.

Primary target: `src/backend/mir/aarch64/codegen/comparison.cpp`

Actions:

- Replace local compare-join and fused-operand provenance reconstruction with
  `PreparedMaterializedCompareJoin*` and
  `PreparedFusedCompareOperandProducer` where those facts already exist.
- Keep compare operand materialization, register selection, condition-code
  spelling, and final AArch64 comparison record construction local.
- Prove materialized compare joins and fused compare operand cases that would
  expose duplicated authority.

Completion check:

- Comparison lowering treats prepared compare join and fused operand facts as
  authoritative, with focused materialized-compare proof green.

### Step 6: Acceptance Validation And Drift Check

Goal: verify the completed route satisfies the source idea without overfitting
or moving target-local policy into shared authority.

Primary targets:

- `plan.md`
- `todo.md`
- implementation files touched by Steps 2 through 5
- `ideas/open/71_aarch64_scalar_control_flow_prepared_authority_cleanup.md`

Actions:

- Review the final diff against the source idea's in-scope, out-of-scope, and
  reviewer reject signals.
- Confirm no expectations were weakened, no supported paths were marked
  unsupported, and no named testcase shortcut was added.
- Confirm AArch64 immediate admissibility, fallback materialization,
  condition-code spelling, scalar/cast record production, compare operand
  materialization, and final branch emission remain target-local.
- Run the supervisor-selected acceptance proof, escalating to regression guard
  or broader CTest when shared prepared lookup behavior or multiple owners were
  changed.
- Record final proof and any remaining blockers in `todo.md`.

Completion check:

- `todo.md` records acceptance proof and drift-check results, and the supervisor
  can decide whether idea 71 is ready for lifecycle close or another runbook
  checkpoint.
