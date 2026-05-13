Status: Active
Source Idea Path: ideas/open/211_aarch64_machine_instruction_node_contract.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Commit The Machine Instruction Node Contract

# Current Packet

## Just Finished

Step 2 of `plan.md` added
`src/backend/mir/aarch64/MACHINE_INSTRUCTION_NODE_CONTRACT.md` as the
canonical structured AArch64 machine-instruction-node boundary.

The contract defines:

- opcode or instruction-family identity, with mnemonic strings explicitly not
  serving as semantic node identity
- typed operand categories for registers, immediates, memory, symbols, branch
  targets, conditions/predicates, prepared values, frame slots, and data
  references
- source metadata that ties nodes back to BIR ids, prepared facts, function
  and block identity, instruction indexes, and family-specific source records
- def/use/clobber/side-effect metadata needed by later validation, scheduling,
  structured peepholes, printers, encoders, and object paths
- the separation between target MIR records, machine instruction nodes, `.s`
  printer output, structured encoder/object input, and external assembler
  parser input

It cites the implemented audited surfaces from Step 1:
`BACKEND_ENTRY_CONTRACT.md`, `module/module.hpp`, `abi/abi.hpp`,
`codegen/records.hpp`, and the text-first compatibility/legacy surfaces in
`codegen/emit.hpp`, `assembler/mod.hpp`, `assembler/parser.hpp`,
`assembler/types.hpp`, and `assembler/encoder/mod.hpp`.

## Suggested Next

Execute Step 3: align implemented AArch64 record surfaces with the new
machine-instruction-node contract. Focus on `codegen/records.hpp`,
`codegen/records.cpp`, `abi/`, and `module/` without adding instruction
selection, assembly printing, encoding, object writing, or linker behavior.

## Watchouts

- Step 3 should treat the new contract as documentation authority and keep
  implementation edits narrow.
- Preserve existing prepared ids, target records, and typed register operands;
  the main issue is clarifying whether each record is target MIR, machine node,
  printer output, or encoder input.
- Do not convert display spellings in `module/` into lookup authority.
- Do not let `codegen -> asm text -> parse_asm -> encoder` survive as an
  internal semantic path.

## Proof

`git diff --check` passed. Proof log: `test_after.log`.
