# Current Packet

Status: Active
Source Idea Path: ideas/open/218_aarch64_structured_asm_encoder_linker_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit AArch64 Boundary Wording

## Just Finished

Completed Step 1 from `plan.md`: audited AArch64 boundary wording for
references to assembler parser input, encoder input, object/linker input,
`machine_printer.cpp`, `.s` text, external `.s`, and external `.ll`.

Classification:

- Already-safe live contracts: `src/backend/mir/aarch64/MACHINE_INSTRUCTION_NODE_CONTRACT.md`,
  `src/backend/mir/aarch64/BACKEND_ENTRY_CONTRACT.md`,
  `src/backend/mir/aarch64/BINARY_UTILS_CONTRACT.md`,
  `src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md`,
  `src/backend/mir/aarch64/codegen/records.md`, and
  `src/backend/mir/aarch64/codegen/machine_printer.hpp`. These already state
  or expose the selected-machine-node printer route as a terminal consumer and
  reject parser recovery as the internal codegen-to-encoder/object bridge.
- Already-safe legacy/codegen guidance: `src/backend/mir/aarch64/ALLOCATION_CONTRACT.md`,
  `src/backend/mir/aarch64/mod.md`, `src/backend/mir/aarch64/CLASSIFICATION_INDEX.md`,
  `src/backend/mir/aarch64/codegen/emit.md`,
  `src/backend/mir/aarch64/codegen/comparison.md`,
  `src/backend/mir/aarch64/codegen/memory.md`, and
  `src/backend/mir/aarch64/codegen/peephole.md`. These use terminal `.s`
  printing or historical text recovery only as rejected/deferred context.
- Historical-only docs: `src/backend/mir/aarch64/assembler/mod.md`,
  `src/backend/mir/aarch64/assembler/parser.md`,
  `src/backend/mir/aarch64/assembler/elf_writer.md`,
  `src/backend/mir/aarch64/assembler/encoder/mod.md`,
  `src/backend/mir/aarch64/assembler/encoder/bitfield.md`,
  `src/backend/mir/aarch64/assembler/encoder/compare_branch.md`,
  `src/backend/mir/aarch64/assembler/encoder/data_processing.md`,
  `src/backend/mir/aarch64/assembler/encoder/fp_scalar.md`,
  `src/backend/mir/aarch64/assembler/encoder/load_store.md`,
  `src/backend/mir/aarch64/assembler/encoder/system.md`,
  `src/backend/mir/aarch64/linker/mod.md`, and
  `src/backend/mir/aarch64/linker/input.md`. These describe removed or staged
  parser/encoder/linker surfaces and mostly already warn not to use printed
  `.s` as backend-owned semantics.
- Ambiguous live headers needing later wording edits:
  `src/backend/mir/aarch64/assembler/mod.hpp`,
  `src/backend/mir/aarch64/assembler/parser.hpp`,
  `src/backend/mir/aarch64/assembler/types.hpp`,
  `src/backend/mir/aarch64/assembler/encoder/mod.hpp`,
  `src/backend/mir/aarch64/codegen/emit.hpp`, and
  `src/backend/mir/aarch64/linker/mod.hpp`. They expose text-first assembler,
  parser, operand, encoder, assemble-module, or object-path linker seams
  without spelling out that these are compatibility, external-input, staged, or
  downstream consumer surfaces rather than the internal printed-`.s` bridge.
- Contract-breaking wording found: none. The live route observed in
  `src/backend/backend.cpp` prints selected AArch64 machine nodes through
  `print_machine_instruction_nodes(...)` and does not feed that output into
  `parse_asm`, encoder, object writer, or linker code.

Concrete later-edit files for Step 2:
`src/backend/mir/aarch64/assembler/mod.hpp`,
`src/backend/mir/aarch64/assembler/parser.hpp`,
`src/backend/mir/aarch64/assembler/types.hpp`,
`src/backend/mir/aarch64/assembler/encoder/mod.hpp`,
`src/backend/mir/aarch64/codegen/emit.hpp`, and
`src/backend/mir/aarch64/linker/mod.hpp`.

## Suggested Next

Execute Step 2 from `plan.md`: mark the ambiguous live headers above so
terminal printer output, external assembler input, structured encoder/object
input, and linker object input are separated explicitly.

## Watchouts

- Keep `machine_printer.cpp` valid as a terminal renderer for `--codegen asm`.
- Preserve the idea 216 external `.c -> .s -> clang/as -> run` smoke path as
  terminal-output validation, not internal backend input.
- Do not implement a full assembler, encoder, object writer, or linker in this
  plan.
- Do not edit the source idea for routine audit findings; record packet state
  here instead.
- Reject any route that parses `machine_printer.cpp` output back into backend
  semantics.
- Search hits in linker emit docs such as `.shstrtab`, `.symtab`, `.strtab`,
  and `text.size()` are false positives for this audit, not `.s` boundary
  wording.

## Proof

Delegated proof completed successfully and wrote `test_after.log`:

```bash
bash -lc 'set -o pipefail; { rg -n "machine_printer|\.s|external \.s|external \.ll|assembler parser|encoder input|object input|linker input|parse(d|r)? assembly|printed assembly|terminal renderer|codegen asm|--codegen asm" src/backend/mir/aarch64 plan.md todo.md; } > test_after.log 2>&1; test -s test_after.log'
```
