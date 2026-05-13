Status: Active
Source Idea Path: ideas/open/211_aarch64_machine_instruction_node_contract.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Audit Existing AArch64 Surfaces

# Current Packet

## Just Finished

Step 1 of `plan.md` audited the AArch64 implemented and markdown surfaces for
machine-instruction-node handoff conflicts.

Implemented record surfaces to align in Step 3:

- `src/backend/mir/aarch64/codegen/records.hpp` already has typed
  `OperandRecord`, `InstructionRecord`, `InstructionPayload`, branch, scalar,
  memory, call, return, assembler, and object record variants. Conflict:
  these are still named as broad record-only instruction records rather than a
  canonical machine-instruction-node boundary, and `RecordSurfaceKind` only
  says `RecordOnly`.
- `src/backend/mir/aarch64/codegen/records.cpp` constructs those instruction
  records from prepared facts without assembly-string parsing. Conflict:
  factory names and `record_surface_kind_name` still describe generic records;
  no surface states that these are the structured machine-node handoff consumed
  by printers or future encoders.
- `src/backend/mir/aarch64/module/module.hpp` and
  `src/backend/mir/aarch64/module/module.cpp` snapshot prepared module,
  function, block, operand, register, frame, branch, call, move, spill/reload,
  data, string, and relocation facts. Conflict: many display spellings
  (`label`, `block_label_text`, `register_name`, `physical_register`,
  `direct_callee_name`, `symbol_label`, `reason`) are present beside ids, so
  Step 3 must keep them display-only and define the downstream handoff through
  typed ids/records, not rendered-name authority.
- `src/backend/mir/aarch64/abi/abi.hpp` and
  `src/backend/mir/aarch64/abi/abi.cpp` provide typed
  `RegisterReference`, register views/banks, prepared-register conversion, and
  target handoff validation. Alignment need: cite these as typed register
  operands for machine nodes while keeping `parse_aarch64_register_name` as a
  conversion/diagnostic helper, not an assembler-parser semantic source.
- `src/backend/mir/aarch64/codegen/emit.hpp` exposes raw BIR/LIR string
  emitters and `assemble_module(...)`. Conflict with the new handoff if it is
  treated as the target-local lowering API; it should be classified as legacy
  compatibility or later printer plumbing.
- `src/backend/mir/aarch64/assembler/mod.hpp`,
  `src/backend/mir/aarch64/assembler/parser.hpp`,
  `src/backend/mir/aarch64/assembler/types.hpp`, and
  `src/backend/mir/aarch64/assembler/encoder/mod.hpp` are implemented
  text-first assembler surfaces: `AssembleRequest::asm_text`, `staged_text`,
  `parse_asm`, `AsmStatement::text`, `Operand::text`, `raw_operands`, and
  `encode_instruction(mnemonic, operands, raw_operands)`. These are direct
  conflicts unless Step 2/4 clearly quarantine them as external assembler or
  legacy assembler inputs, not codegen-owned semantic handoff.

Markdown artifacts needing Step 2/4 contract alignment:

- Top-level current-contract docs: `BACKEND_ENTRY_CONTRACT.md` mostly agrees
  with the desired model; `BIR_PREPARED_GAP_LEDGER.md` already rejects raw
  emitters/parser recovery but still records `emit.hpp` and text-first
  assembler headers as current surfaces; `CLASSIFICATION_INDEX.md` marks the
  legacy text emitter, peephole, parser, and encoder artifacts as obsolete or
  deferred. Step 4 should update/cross-reference these to the machine-node
  contract.
- `BINARY_UTILS_CONTRACT.md` is the strongest top-level conflict. It describes
  `HIR -> LIR -> AArch64 assembly text`, `assembly text -> clang`, a
  text-first assembler inventory, `backend text emission -> parse_asm(raw
  text) -> assemble(...)`, and says later work should treat that shape as the
  compatibility baseline. Step 4 must narrow that to external-toolchain or
  compatibility behavior only.
- `mod.md` and `codegen/mod.md` still describe target lowering as
  assembly-text emission and keep old `emit`/`peephole` structure visible.
- `codegen/emit.md`, `codegen/asm_emitter.md`, and `codegen/peephole.md` are
  text-route artifacts. `emit.md` documents `emit_module(...)` returning
  strings and `assemble_module(...)` re-entering the assembly seam;
  `asm_emitter.md` is the legacy assembly text emitter; `peephole.md` is a
  line-oriented assembly-text optimizer. These must not define backend
  semantics.
- Other `codegen/*.md` artifacts that need wording cleanup before future work:
  `alu.md`, `calls.md`, `cast_ops.md`, `comparison.md`, `float_ops.md`,
  `globals.md`, `inline_asm.md`, `memory.md`, `prologue.md`, `returns.md`,
  and `variadic.md` contain assembly-shape, final assembly text, or operand
  substitution language. `atomics.md`, `f128.md`, `i128_ops.md`, and
  `intrinsics.md` are deferred feature notes that should stay blocked from
  inventing text-emitter routes.
- `assembler/mod.md` and `assembler/parser.md` are direct conflict artifacts:
  they document `parse_asm(request.asm_text)`, `AsmStatement`, raw operand
  reconstruction, literal-pool expansion, numeric-label resolution, and parser
  operand recovery before object writer staging.
- `assembler/elf_writer.md` depends on `AsmStatement` from the parser,
  parser-split operands, and `encoder::encode_instruction`; it should become
  object-writer history or a future structured-node/object consumer.
- `assembler/encoder/mod.md` and encoder family docs
  `bitfield.md`, `compare_branch.md`, `data_processing.md`, `fp_scalar.md`,
  `load_store.md`, `neon.md`, and `system.md` all describe encoders consuming
  parser-shaped `Operand`/`raw_operands` or string register helpers. Some
  encoding facts may be reusable, but Step 4 must state that a rebuilt encoder
  consumes machine nodes or lower structured encoding records.
- Linker docs `linker/elf.md`, `linker/mod.md`, and `linker/reloc.md` are not
  codegen handoff owners, but they mention parser aliases, object parsers, or
  instruction encoder relocation output. Step 4 should make linker/object
  expectations consume object/relocation records, never assembly text recovered
  from codegen.

## Suggested Next

Execute Step 2: add the canonical machine-instruction-node contract under
`src/backend/mir/aarch64/`, defining the structured node boundary and citing
the implemented typed record surfaces above while explicitly quarantining
text-first assembler/parser/encoder surfaces as external or legacy inputs.

## Watchouts

- Step 2 should not edit implementation yet; it should define the contract that
  Step 3 and Step 4 can cite.
- Preserve the useful typed facts in `records.hpp`, `module.hpp`, and
  `abi.hpp`; the conflict is boundary naming/authority, not that these records
  are text emitters.
- Treat `emit.hpp`, assembler parser/types, encoder helpers, and
  `BINARY_UTILS_CONTRACT.md` as compatibility or legacy surfaces until they
  are explicitly rebuilt around machine nodes.
- Do not let `codegen -> asm text -> assembler parser` survive as an internal
  semantic path.

## Proof

`git diff --check` passed. Proof log: `test_after.log`.
