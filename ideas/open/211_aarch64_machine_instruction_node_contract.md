# AArch64 Machine Instruction Node Contract

Status: Open
Created: 2026-05-13

Depends On:
- `ideas/closed/207_aarch64_target_register_and_instruction_record_core.md`
- `ideas/closed/208_aarch64_branch_compare_target_mir_records.md`
- `ideas/closed/209_aarch64_scalar_alu_cast_first_instruction_slice.md`
- `ideas/closed/210_aarch64_memory_operand_model_from_prepared_facts.md`

## Goal

Define and apply the AArch64 machine-instruction node contract between MIR
codegen and assembler/encoder layers.

The backend must not use assembly text as the internal semantic handoff between
codegen and assembler. Codegen should produce typed machine instruction nodes;
`.s` output should be a printer/debug/external-assembler consumer of those
nodes, and any future built-in encoder/object writer should consume the same
structured nodes or a deliberately lower structured encoding layer.

## Why This Idea Exists

The ARM reference backend under `ref/claudes-c-compiler/src/backend/arm` uses a
text-oriented path: codegen emits assembly text, and assembler/parser surfaces
recover operands from text. That layout is useful as a responsibility
reference, but the text handoff is not acceptable for this rebuild.

Ideas 207-210 are growing target-local AArch64 record surfaces in `.cpp/.hpp`.
Now is the right time to make the next boundary explicit before those records
drift into either text-emitter helpers or assembler-parser-shaped operands.

The desired model is:

```text
BIR / PreparedBirModule
  -> AArch64 target MIR records
  -> AArch64 machine instruction nodes
  -> optional .s printer
  -> optional structured encoder / object writer
```

Assembly text is output, not the backend's internal semantic representation.

## In Scope

- Audit the currently implemented AArch64 `.cpp/.hpp` record surfaces,
  especially:
  - `src/backend/mir/aarch64/codegen/records.hpp`
  - `src/backend/mir/aarch64/codegen/records.cpp`
  - `src/backend/mir/aarch64/abi/`
  - `src/backend/mir/aarch64/module/`
- Update implemented `.cpp/.hpp` records so their naming and structure clearly
  fit a machine-instruction-node pipeline rather than an assembly-text emitter
  pipeline.
- Define the machine instruction node contract, including:
  - opcode or instruction-family enum
  - typed operand variants for register, immediate, memory, symbol, branch
    target, condition/predicate, prepared value, frame slot, and data reference
  - source metadata tying nodes back to BIR/prepared facts
  - def/use/clobber/side-effect metadata where needed by later passes
  - explicit distinction between target MIR records, final machine instruction
    nodes, `.s` printer output, and encoder/object input
- Update not-yet-implemented AArch64 markdown artifacts so they no longer imply
  codegen should hand assembly text to assembler/parser:
  - `src/backend/mir/aarch64/codegen/*.md`
  - `src/backend/mir/aarch64/assembler/*.md`
  - `src/backend/mir/aarch64/assembler/encoder/*.md`
  - `src/backend/mir/aarch64/linker/*.md` where linker/object expectations
    mention assembly text as an upstream semantic source
  - top-level `BACKEND_ENTRY_CONTRACT.md`, `BIR_PREPARED_GAP_LEDGER.md`, and
    `CLASSIFICATION_INDEX.md` as needed
- Define where the `.s` printer belongs and state that it is a consumer of
  machine instruction nodes, not an owner of backend semantics.
- Define where a future built-in encoder belongs and state whether it consumes
  machine instruction nodes directly or a lower structured encoding record.
- Add focused tests or compile proof that implemented records can construct or
  reference machine instruction nodes without assembly-string parsing.

## Out Of Scope

- Emitting real assembly text for scalar, memory, branch, call, or return
  operations.
- Implementing instruction encoding, object writing, or linking.
- Completing all opcode families in one slice.
- Rewriting the active 210 memory model beyond the minimum naming/contract
  adjustments needed to align with machine instruction nodes.
- Parsing assembly text to recover operands for codegen.
- Adding peephole passes; later peepholes should be structured machine-node
  passes, not text rewrites.

## Acceptance Criteria

- A committed markdown contract under `src/backend/mir/aarch64/` defines the
  machine instruction node boundary between codegen and assembler/encoder.
- Implemented `.cpp/.hpp` AArch64 record surfaces are updated to align with
  the node contract and do not imply assembly text is the internal handoff.
- Not-yet-implemented markdown artifacts are updated so future work follows
  the machine-node pipeline rather than `codegen -> assembly text -> parser`.
- The contract clearly places:
  - target MIR records
  - machine instruction nodes
  - optional `.s` printer
  - optional structured encoder/object writer
- No codegen path emits assembly text as semantic output for the assembler to
  parse back into operands.
- Focused tests or compile proof cover at least representative construction or
  ownership of machine instruction nodes from existing target records.

## Reviewer Reject Signals

- The route preserves or introduces an internal `codegen -> asm text ->
  assembler parser` handoff.
- The change only renames text-emitter helpers without defining structured
  machine instruction nodes and operands.
- Implemented `.cpp/.hpp` records still make assembly strings the primary
  downstream representation.
- Markdown artifacts keep directing future implementation toward text
  peepholes, parser operand recovery, or assembly-string semantic recovery.
- The route expands into broad instruction selection, assembly printing,
  encoding, object writing, or linker behavior before the node contract is
  accepted.
- Machine instruction operands recover semantic facts from rendered names,
  printed BIR, old markdown examples, or parsed assembly text instead of BIR
  and prepared ids.
