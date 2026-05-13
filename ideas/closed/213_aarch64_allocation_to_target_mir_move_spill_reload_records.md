# AArch64 Allocation To Target MIR Move Spill Reload Records

Status: Closed
Created: 2026-05-13
Closed: 2026-05-13

Depends On:
- `ideas/closed/210_aarch64_memory_operand_model_from_prepared_facts.md`
- `ideas/closed/211_aarch64_machine_instruction_node_contract.md`
- `ideas/closed/212_bir_mir_allocation_contract.md`

## Goal

Implement the target-MIR record layer that consumes the allocation contract and
prepared move/regalloc facts for AArch64 moves, parallel copies, ABI bindings,
spills, and reloads.

This idea turns the closed allocation contract into structured target-MIR facts.
It does not select final `ldr`, `str`, `mov`, call, return, or prologue machine
instructions.

## Why This Idea Exists

Ideas 210-212 closed the memory operand, machine-node, and allocation
boundaries. The remaining gap is the bridge where target MIR sees a value home
that may be a physical register, structured spill slot, non-register prepared
location, or future virtual placeholder.

The backend needs explicit records for:

```text
allocation result / prepared regalloc
  -> target MIR move / copy / spill / reload / ABI-binding records
  -> later machine instruction nodes
```

Without this layer, scalar, memory, branch, call, return, and prologue work
would each be tempted to allocate scratch registers, invent spill slots, or
materialize reload/store behavior locally.

## In Scope

- Add or refine AArch64 target-MIR records for:
  - physical-register value locations
  - structured spill-slot locations
  - reload pseudo operations from spill slot to reserved MIR scratch register
  - spill/store pseudo operations from register or scratch to spill slot
  - prepared moves and parallel-copy bundles
  - ABI-binding moves between prepared locations and argument/return resources
  - deferred or fail-closed future virtual-register placeholders
- Source these records from `PreparedBirModule`, `PreparedValueLocations`,
  `PreparedRegalloc`, `PreparedStoragePlans`, `PreparedMoveBundle`,
  `PreparedAbiBinding`, and `PreparedSpillReloadOp` where available.
- Consume `src/backend/mir/aarch64/ALLOCATION_CONTRACT.md` as the authority for
  physical homes, structured spill slots, reserved scratch pools, and
  fail-closed scratch exhaustion.
- Preserve provenance back to prepared value ids, BIR value/name ids, function,
  block, instruction, register class, type kind, and prepared facts.
- Update `src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md` and relevant
  codegen markdown so later instruction-selection ideas know these records
  exist.
- Add focused tests or compile proof for representative physical-register,
  spill-slot, reload, spill/store, move, and ABI-binding records.

## Out Of Scope

- Selecting or emitting AArch64 machine instructions.
- Implementing `ldr`, `str`, `mov`, prologue/epilogue, call, return, or branch
  machine nodes.
- Implementing a full register allocator or register scavenger.
- Assigning final stack-pointer/frame-pointer offsets to spill slots.
- Treating assembly text or rendered names as allocation authority.
- Reworking the allocator algorithm inside shared preparation.

## Acceptance Criteria

- AArch64 target MIR has structured records for move, parallel copy,
  spill/reload, and ABI-binding facts sourced from prepared allocation data.
- Structured spill-slot ids remain distinct from final stack offsets.
- Reserved MIR scratch usage is represented according to
  `ALLOCATION_CONTRACT.md`, and scratch exhaustion fails closed or opens a
  follow-up register-scavenger idea.
- Later scalar/memory/branch/call/return slices can consume these records
  instead of allocating registers or inventing spill materialization locally.
- Focused tests or compile proof cover representative records without assembly
  text parsing.

## Completion Note

Closed after the active runbook added structured AArch64 target-MIR records for
allocation locations, moves, parallel copies, ABI bindings, and spill/reload
pseudo operations, updated the allocation-sensitive markdown consumers, and
proved the representative record surfaces with backend structured-record tests.

Close-time regression guard used the backend scope:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
The monotonic checker passed in documented non-decreasing mode with 131 passed
tests before and after, no new failures, and no suspicious timeouts.

## Reviewer Reject Signals

- The slice emits final load/store/move assembly or encodes instructions.
- Spill slots become rendered stack-address strings or concrete offsets before
  frame layout owns them.
- Scalar, memory, branch, call, or return lowering still owns ad hoc register
  allocation or scratch selection.
- Missing prepared allocation facts are patched around inside AArch64 target
  code instead of becoming a separate gap idea.
- Records recover allocation semantics from printed BIR, rendered register
  names, or parsed assembly text.
