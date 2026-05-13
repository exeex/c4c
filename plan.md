# AArch64 Allocation To Target MIR Move Spill Reload Records

Status: Active
Source Idea: ideas/open/213_aarch64_allocation_to_target_mir_move_spill_reload_records.md

## Purpose

Turn the accepted allocation contract and prepared regalloc facts into explicit
AArch64 target-MIR records for value homes, moves, parallel copies,
spill/reload pseudo operations, and ABI bindings.

## Goal

Make later AArch64 scalar, memory, branch, call, return, and prologue work
consume structured target-MIR allocation records instead of allocating
registers, inventing spill slots, or recovering facts from text.

## Core Rule

Target MIR may record allocation facts and pseudo operations, but this plan
must not select final `ldr`, `str`, `mov`, call, return, prologue, or assembly
forms.

## Read First

- `ideas/open/213_aarch64_allocation_to_target_mir_move_spill_reload_records.md`
- `src/backend/mir/aarch64/ALLOCATION_CONTRACT.md`
- `src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md`
- `src/backend/mir/aarch64/module/module.hpp`
- `src/backend/mir/aarch64/module/module.cpp`
- `src/backend/prealloc/prealloc.hpp`

## Current Targets

- `src/backend/mir/aarch64/module/module.hpp`
- `src/backend/mir/aarch64/module/module.cpp`
- `src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md`
- relevant AArch64 codegen markdown that still implies local allocation,
  local spill materialization, or assembly-text authority
- focused tests or compile proof covering representative record construction

## Non-Goals

- Do not emit or select final load/store/move/call/return/prologue machine
  instructions.
- Do not assign final frame-pointer or stack-pointer offsets to spill slots.
- Do not implement a full allocator, register scavenger, or frame layout pass.
- Do not patch missing prepared facts inside AArch64 target code.
- Do not recover allocation semantics from rendered names, printed BIR, LIR
  text, parsed assembly, or legacy codegen examples.

## Working Model

The accepted handoff is:

```text
PreparedBirModule / prepared allocation facts
  -> AArch64 target-MIR allocation, move, spill/reload, and ABI-binding records
  -> later AArch64 machine instruction nodes
  -> optional printer / encoder / object output
```

The first target-MIR layer should preserve prepared provenance:
`PreparedValueId`, `ValueNameId`, function, block, instruction, register
class, type kind, prepared value homes, storage plans, regalloc assignments,
move bundles, ABI bindings, and spill/reload ops.

Structured spill-slot ids remain allocation facts. Concrete stack offsets are
prepared snapshots only until frame layout owns final layout.

## Execution Rules

- Keep changes behavior-preserving unless a step explicitly tightens a record
  invariant.
- Prefer adding record fields, provenance, and validation over broad rewrites.
- When a required shared prepared carrier is missing, open a separate
  `ideas/open/` gap instead of inventing AArch64-local truth.
- Preserve `ALLOCATION_CONTRACT.md` scratch policy: reserved MIR scratch
  registers are excluded from long-lived homes and scratch exhaustion must fail
  closed or become a separate register-scavenger idea.
- Each code-changing step needs fresh build or compile proof. Add focused
  record tests where the repo already has a suitable test surface.

## Steps

### Step 1: Inspect Existing Record Coverage

Goal: establish the exact current coverage for target-MIR allocation records.

Primary target:
`src/backend/mir/aarch64/module/module.hpp` and
`src/backend/mir/aarch64/module/module.cpp`

Actions:

- Inspect `TargetRegisterRecord`, `OperandRecord`, `MoveRecord`,
  `AbiBindingRecord`, `SpillReloadRecord`, and `ParallelCopyRecord`.
- Compare current fields against `ALLOCATION_CONTRACT.md` requirements for
  physical homes, structured spill slots, reserved scratch, future virtual
  placeholders, non-register prepared locations, and provenance.
- Identify whether existing stack offset fields are clearly marked as prepared
  snapshots rather than final frame-layout authority.
- Identify any AArch64 codegen markdown that still tells feature slices to
  allocate registers, synthesize spills locally, or use assembly text as
  allocation authority.

Completion check:
`todo.md` records the specific record gaps and the smallest next code/docs
targets, without editing implementation files in this step unless the executor
chooses to combine inspection with a narrow fix.

### Step 2: Tighten Allocation Location And Register Records

Goal: make value-home and physical-register records carry enough structured
allocation authority for later consumers.

Primary target:
`src/backend/mir/aarch64/module/module.hpp` and
`src/backend/mir/aarch64/module/module.cpp`

Actions:

- Preserve register class, bank, view or width, occupied registers, allocation
  pool where known, value id/name, type kind, and source prepared fact pointers.
- Distinguish value homes, regalloc assignments, spill authority, storage-plan
  locations, reserved scratch locations, display-only names, and future
  placeholders where the source facts justify them.
- Keep long-lived homes separate from reserved MIR scratch records.
- Keep spill-slot ids structured and do not convert them into final offsets.
- Add fail-closed handling or a follow-up open idea if required scratch
  authority is unavailable.

Completion check:
Representative physical-register, spill-slot, non-register, and future/deferred
locations are representable without assembly text or local allocation.

### Step 3: Tighten Move, Parallel-Copy, And ABI-Binding Records

Goal: make prepared movement facts directly consumable by later instruction
selection.

Primary target:
`src/backend/mir/aarch64/module/module.hpp` and
`src/backend/mir/aarch64/module/module.cpp`

Actions:

- Preserve `PreparedMoveBundle`, `PreparedMoveResolution`,
  `PreparedAbiBinding`, and parallel-copy provenance.
- Preserve move phase, authority kind, source/destination value ids,
  destination kind, destination storage kind, ABI index, register/stack target
  snapshots, cycle-temp use, coalescing, execution site, and reason.
- Ensure ABI-binding records remain movement records, not call/return
  instruction-selection policy.
- Keep join-copy and parallel-copy records separate from scalar or branch
  lowering.

Completion check:
Representative prepared moves, out-of-SSA parallel copies, cycle-temp moves,
and ABI-binding records can be observed as target-MIR records with provenance.

### Step 4: Tighten Spill And Reload Pseudo Records

Goal: represent spill/store and reload pseudo operations before machine-node
selection.

Primary target:
`src/backend/mir/aarch64/module/module.hpp` and
`src/backend/mir/aarch64/module/module.cpp`

Actions:

- Preserve `PreparedSpillReloadOp` provenance, value id, op kind, function,
  block, instruction, register class or bank, scratch register authority,
  occupied scratch registers, spill-slot id, and prepared stack-offset snapshot
  where present.
- Make reload-to-scratch and spill/store-from-register semantics explicit as
  target-MIR pseudo records only.
- Reject final `ldr`, `str`, `mov`, prologue, or stack-offset materialization
  in this step.
- If reserved scratch selection cannot be represented from prepared facts,
  fail closed or create a separate register-scavenger/gap idea.

Completion check:
Representative spill and reload records preserve spill-slot ids and scratch
authority without selecting final instructions or rendered stack addresses.

### Step 5: Update Ledgers And Markdown Consumers

Goal: make later AArch64 slices discover the new record layer and avoid local
allocation workarounds.

Primary target:
`src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md` and relevant
`src/backend/mir/aarch64/codegen/*.md`

Actions:

- Update the gap ledger so move, parallel-copy, ABI-binding, spill, and reload
  record availability matches the implemented target-MIR surface.
- Mark any missing prepared carriers as separate open-idea candidates rather
  than AArch64-local workarounds.
- Remove or narrow markdown language that implies scalar, memory, branch,
  call, return, or prologue slices may allocate registers or invent spills.
- Keep final instruction selection and ABI call/return/frame contracts assigned
  to later ideas.

Completion check:
Markdown points later feature slices at the structured records and preserves
the out-of-scope boundaries from the source idea.

### Step 6: Add Focused Proof

Goal: prove the record layer handles representative allocation facts.

Primary target:
the repo-native AArch64 module/codegen test or compile surface selected by the
executor.

Actions:

- Add or update focused coverage for physical-register homes, spill-slot
  homes, spill/reload records, prepared moves, parallel-copy records, and
  ABI-binding records.
- Prefer structured record assertions over assembly text matching.
- Run the narrow build/test command selected by the supervisor.
- Escalate to a broader validation checkpoint if multiple codegen buckets or
  shared prepared carriers changed.

Completion check:
Fresh proof covers representative records without assembly parsing, expectation
downgrades, or named-case-only shortcuts.

## Completion Criteria

- AArch64 target MIR has structured records for move, parallel copy,
  spill/reload, and ABI-binding facts sourced from prepared allocation data.
- Structured spill-slot ids remain distinct from final stack offsets.
- Reserved MIR scratch usage follows `ALLOCATION_CONTRACT.md`, and any scratch
  exhaustion path fails closed or is split into a follow-up idea.
- Later AArch64 lowering slices can consume these records instead of allocating
  registers or inventing spill behavior locally.
- Focused proof covers representative records without assembly-text parsing.
