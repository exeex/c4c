# Current Packet

Status: Active
Source Idea Path: ideas/open/218_aarch64_structured_asm_encoder_linker_contract.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Define Structured ASM/Encoding Record Surface

## Just Finished

Completed Step 3 from `plan.md`: defined the minimum future structured
asm/encoding record surface in the accepted AArch64 contract docs.

Updated contract docs:

- `src/backend/mir/aarch64/MACHINE_INSTRUCTION_NODE_CONTRACT.md`: added the
  accepted translation-unit, section, label, operator/instruction, directive,
  data, typed operand, and `RelocationNeed`-style record families; included
  the concrete `AsmRegisterRef`, `AsmRegisterUseKind`, `AsmRegisterUse`,
  `AsmValueProvenance`, `AsmAllocationProvenance`, and `AsmRegisterOperand`
  decomposition; and named the semantic payload that must survive toward
  object/linker records.
- `src/backend/mir/aarch64/codegen/records.md`: mirrored the minimum record
  families and register operand separation so the current record-core roadmap
  points future encoder/object work at readable structured records instead of
  a catch-all operand payload.

## Suggested Next

Execute Step 4 from `plan.md`: tie structured asm/encoding records to prepared
lifecycle authority and post-selection machine-level effects.

## Watchouts

- Keep Step 4 in contract/design wording unless the supervisor explicitly
  delegates a live code surface.
- Preserve the separation between terminal printer output, external assembler
  input, structured encoder/object records, and linker object inputs.
- Prepared lifecycle facts are upstream authority, but structured
  asm/encoding records still need post-selection target-machine effects such
  as implicit register uses/defs, flags, scratch lifetimes, selected opcode
  clobbers, and section/relocation ownership.
- Do not edit the source idea for routine execution details; record packet
  state here instead.

## Proof

Delegated proof completed successfully and wrote `test_after.log`:

```bash
bash -lc 'set -o pipefail; rg -n "AsmRegisterRef|AsmRegisterUseKind|AsmValueProvenance|AsmAllocationProvenance|AsmRegisterOperand|translation-unit|RelocationNeed|structured asm/encoding" src/backend/mir/aarch64/MACHINE_INSTRUCTION_NODE_CONTRACT.md src/backend/mir/aarch64/codegen/records.md todo.md > test_after.log 2>&1; test -s test_after.log'
```

Proof result: required structured asm/encoding contract vocabulary is present.
