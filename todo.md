# Current Packet

Status: Active
Source Idea Path: ideas/open/218_aarch64_structured_asm_encoder_linker_contract.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Centralize Enum Spelling Contract

## Just Finished

Completed Step 5 from `plan.md`: documented the centralized enum spelling
contract for future structured asm/encoding record surfaces without adding live
enum helpers.

Updated contract docs:

- `src/backend/mir/aarch64/MACHINE_INSTRUCTION_NODE_CONTRACT.md`: added the
  enum-to-string/to_string mapping requirement for future section, label,
  directive, operator/opcode, operand kind, register use kind, relocation kind,
  and record surface enums; terminal printers and diagnostics must call those
  mappings instead of scattering spelling switches through lowering code.
- `src/backend/mir/aarch64/codegen/records.md`: mirrored the centralized enum
  spelling rule for future record surfaces and noted that this docs-only plan
  introduced no live enum helper requirement.

## Suggested Next

Execute the next supervisor-selected plan step after Step 5, keeping enum
spelling as display-only support around typed structured records.

## Watchouts

- Preserve the separation between terminal printer output, external assembler
  input, structured encoder/object records, and linker object inputs.
- New future enum kinds for sections, labels, directives, operator/opcode
  categories, operand kinds, register use kinds, relocation kinds, and record
  surfaces need one centralized display mapping family.
- Lowering and record construction should use typed enum values; terminal
  printers and diagnostics should be the consumers of enum display spelling.
- Do not edit the source idea for routine execution details; record packet
  state here instead.

## Proof

Delegated proof completed successfully and wrote `test_after.log`:

```bash
bash -lc 'set -o pipefail; rg -n "enum-to-string|to_string mapping|section.*enum|label.*enum|directive.*enum|operator/opcode|operand kind|register use kind|relocation kind|record surface|terminal printers and diagnostics|AsmRegisterUseKind" src/backend/mir/aarch64/MACHINE_INSTRUCTION_NODE_CONTRACT.md src/backend/mir/aarch64/codegen/records.md todo.md > test_after.log 2>&1; test -s test_after.log'
```

Proof result: required centralized enum spelling contract vocabulary is
present, and this docs-only slice requires no live enum mapping helper.
