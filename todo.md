Status: Active
Source Idea Path: ideas/open/211_aarch64_machine_instruction_node_contract.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Align Implemented Record Surfaces

# Current Packet

## Just Finished

Step 3 of `plan.md` aligned the implemented AArch64 codegen record surface
with `MACHINE_INSTRUCTION_NODE_CONTRACT.md`.

`RecordSurfaceKind` now distinguishes target MIR records, structured machine
instruction nodes, printer output, structured encoder input, and external
assembler input. Existing `RecordOnly` initializers remain as a compatibility
alias for target MIR/pre-node records.

The instruction record factories now mark branch, scalar, memory, call, and
return wrappers as structured downstream machine-instruction-node records.
Assembler wrappers are marked as external assembler input, and object wrappers
are marked as structured encoder input. Payload records still preserve prepared
ids, BIR opcodes, typed operands, branch targets, memory facts, call facts, and
data/symbol identity without adding selection, printing, encoding, object
writing, linking, or text parsing.

Focused backend record tests were updated to cover the new surface spellings
and helper predicates.

## Suggested Next

Execute Step 4: align AArch64 markdown roadmap artifacts with the structured
machine-node pipeline. Keep `.s` printing and external assembler parsing as
consumers or external-input paths, not internal semantic handoff routes.

## Watchouts

- `RecordSurfaceKind::RecordOnly` remains available only as a compatibility
  spelling for target MIR/pre-node records; new tests should prefer
  `TargetMirRecord`, `MachineInstructionNode`, `EncoderInput`, or
  `ExternalAssemblerInput`.
- This slice did not edit the text-first assembler/parser/encoder
  implementation surfaces; Step 4 should handle roadmap markdown language for
  those paths.
- Module display labels remain display/diagnostic fields and were not promoted
  as lookup authority.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
passed. Proof log: `test_after.log`.
