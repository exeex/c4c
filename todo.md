Status: Active
Source Idea Path: ideas/open/219_aarch64_natural_operator_naming_and_printer_spelling.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Current AArch64 Naming And Spelling Surfaces

# Current Packet

## Just Finished

Completed Step 1: audited current AArch64 naming and spelling surfaces without
editing implementation files.

Audited files:
- `src/backend/mir/aarch64/codegen/records.hpp`
- `src/backend/mir/aarch64/codegen/records.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `src/backend/mir/aarch64/assembler/types.hpp`
- `src/backend/mir/aarch64/assembler/mod.hpp`
- `src/backend/mir/aarch64/assembler/parser.hpp`
- `src/backend/mir/aarch64/assembler/encoder/mod.hpp`
- `src/backend/mir/aarch64/codegen/emit.hpp`
- focused AArch64 coverage under `tests/backend/backend_aarch64_*`, especially
  `backend_aarch64_machine_printer_test.cpp`,
  `backend_aarch64_target_instruction_records_test.cpp`,
  `backend_aarch64_scalar_alu_records_test.cpp`,
  `backend_aarch64_scalar_cast_records_test.cpp`,
  `backend_aarch64_memory_operand_records_test.cpp`, and
  `backend_aarch64_branch_compare_contract_test.cpp`.

Supported naming surfaces found:
- Stream/surface and family names already live in `RecordSurfaceKind`,
  `InstructionFamily`, and their `*_name` helpers in `records.cpp`.
- Machine-node operator-like names currently live in `MachineOpcode`,
  `MachinePseudoKind`, `MemoryInstructionKind`, `ScalarAluOperationKind`,
  `ScalarCastOperationKind`, `BranchConditionForm`, and
  `BranchCompareCandidateKind`.
- Diagnostic strings for enum/operator-like names mostly use centralized
  `*_name` helpers in `records.cpp`, and focused tests assert several of those
  spellings.
- Printer mnemonic spelling for the live supported subset is local to
  `machine_printer.cpp`: `mov`, `ret`, `b`, `cbnz`, `str`, and `ldr`.
- External assembler compatibility names remain text-shaped in
  `assembler/types.hpp` (`AsmStatementKind`, `op`, raw operand text) and
  `encoder/mod.hpp` (`RelocType`, `EncodeResultKind`, `encode_instruction`
  taking a mnemonic string). These are not the internal machine-node handoff.

Shared spelling-source status:
- Supported diagnostic enum names have shared sources in `records.cpp`.
- Supported printer mnemonics do not yet have a shared source with
  machine-node/operator names; they are duplicated as printer literals and in
  expected test strings.
- There is no live `AArch64OperatorKind` / structured operator enum yet, so the
  current natural operator layer is represented indirectly by the machine-node
  opcode and operation-kind enums.

Concrete drift or duplication targets for later steps:
- Introduce or normalize one helper/table for supported machine-node/operator
  mnemonic spelling before routing printer diagnostics through it.
- Reconcile generic names such as `MachineOpcode::Branch`,
  `ConditionalBranch`, `CompareBranch`, `Load`, `Store`, `SignExtend`,
  `ZeroExtend`, `Truncate`, `SpillToSlot`, and `ReloadFromSlot` with natural
  printer spellings such as `b`, `cbnz`, `ldr`, `str`, `mov`, and `ret` without
  moving encoder canonicalization into MIR lowering.
- Keep `ScalarAluOperationKind` spelling (`add`, `sub`, `and`, `or`, `xor`)
  centralized while deciding whether those become the supported natural
  operator spelling source or remain target-MIR record vocabulary.
- Add drift guards around shared helper output versus printer text for the
  currently printable subset; existing tests assert literal printer strings but
  do not prove the literals came from the shared enum/operator spelling source.

## Suggested Next

Execute Step 2: document the naming tiers and alias ownership in the contract
markdown before introducing shared spelling helpers.

## Watchouts

- Keep idea 218's boundary intact: printed `.s` is terminal renderer output,
  not semantic backend input.
- Do not touch `ideas/open/220_aarch64_markdown_driven_backend_case_bringup.md`.
- Do not expand instruction selection coverage just to add more operator names.
- Do not downgrade expectations or introduce testcase-shaped shortcuts.
- The live printer has hard-coded mnemonics; changing those before Step 2
  documents the tier boundaries risks mixing printer spelling with semantic
  machine-node identity.
- External assembler and encoder headers still accept mnemonic strings for the
  external text path only; do not treat those as the internal shared operator
  source.

## Proof

Step 1 audit proof:

```bash
bash -lc 'set -o pipefail; { rg -n "enum class|MachineOpcode|MachinePseudoKind|InstructionFamily|RecordSurfaceKind|to_string|spelling|mnemonic|print_|diagnostic|operator|opcode|kind_to|string_view" src/backend/mir/aarch64 tests/backend -g "*.hpp" -g "*.cpp" -g "*.md"; } > test_after.log 2>&1; test -s test_after.log'
```

This audit-only packet changed `todo.md` only. The delegated text-search proof
is sufficient for the Step 1 classification and writes `test_after.log`.
