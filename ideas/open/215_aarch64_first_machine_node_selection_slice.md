# AArch64 First Machine Node Selection Slice

Status: Open
Created: 2026-05-13

Depends On:
- `ideas/closed/209_aarch64_scalar_alu_cast_first_instruction_slice.md`
- `ideas/closed/210_aarch64_memory_operand_model_from_prepared_facts.md`
- `ideas/closed/211_aarch64_machine_instruction_node_contract.md`
- `ideas/closed/213_aarch64_allocation_to_target_mir_move_spill_reload_records.md`
- `ideas/closed/214_aarch64_aapcs64_call_return_frame_mir_contract.md`

## Goal

Implement the first conservative AArch64 machine-instruction-node selection
slice over already accepted target-MIR records.

This is the first slice that may create structured machine instruction nodes,
but it must still avoid assembly text emission, encoding, object writing, and
linking.

## Why This Idea Exists

The backend now has record-only scalar, branch, memory operand, allocation, and
machine-node contracts. After the move/spill/reload and AAPCS64 call/frame
boundaries are accepted, the next useful milestone is a small end-to-end
structured selection path:

```text
PreparedBirModule / target MIR records
  -> selected AArch64 machine instruction nodes
  -> proof via structured records
```

The first slice should be intentionally narrow and RISC-like. It should prove
the architecture without trying to complete the backend.

## In Scope

- Select representative machine instruction nodes for a small set of already
  modeled target-MIR records, such as:
  - scalar integer ALU records from idea 209
  - simple cast records where already represented
  - branch/compare records from idea 208
  - reload/spill/store pseudo records from idea 213
  - memory operand records from idea 210 only as operands to structured load
    or store nodes when required by spill/reload materialization
- Use physical registers, reserved MIR scratch records, structured spill-slot
  ids, prepared value provenance, and typed memory operands from the accepted
  contracts.
- Define minimal opcode/family enums or node variants needed for the selected
  subset.
- Add focused tests or compile proof that machine nodes can be constructed and
  inspected without rendering or parsing assembly text.
- Update roadmap markdown to distinguish selected machine nodes from target
  MIR records and from future `.s` printer/encoder consumers.

## Out Of Scope

- Printing `.s`, parsing assembly, encoding instructions, object writing, or
  linking.
- Full scalar ALU, memory, branch, call, return, prologue, FP, vector, atomics,
  intrinsic, variadic, or inline-asm coverage.
- Peephole optimization or CISC-style folding.
- Register allocation, register scavenging, or frame-layout implementation.
- Any testcase-shaped shortcut that only recognizes one named fixture.

## Acceptance Criteria

- A small accepted subset of target-MIR records lowers to structured AArch64
  machine instruction nodes.
- Nodes preserve opcode/family identity, typed operands, source provenance, and
  def/use/clobber/side-effect metadata required by
  `MACHINE_INSTRUCTION_NODE_CONTRACT.md`.
- Spill/reload selection consumes the records from idea 213 and respects
  reserved MIR scratch policy.
- The selected subset is proven by focused tests or compile proof without
  assembly text as an internal semantic handoff.
- Unsupported records fail closed with clear diagnostics or deferred status.

## Reviewer Reject Signals

- The slice emits assembly text or encodes object bytes as its primary proof.
- Selection bypasses target-MIR records and reads directly from rendered BIR,
  old markdown examples, or assembly strings.
- Spill/reload handling invents scratch registers or stack offsets locally
  instead of consuming ideas 212 and 213.
- The route expands into broad backend completion before the first structured
  node path is stable.
- Tests prove only a named fixture through shape matching rather than a general
  lowering rule for the accepted subset.
