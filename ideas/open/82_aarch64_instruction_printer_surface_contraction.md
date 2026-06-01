# AArch64 Instruction And Printer Surface Contraction

## Goal

Reduce target-local machine-record and printer surface duplication in
`instruction.*` and `machine_printer.cpp` without moving AArch64 printer
spelling or record-schema policy into shared BIR.

## Why This Exists

The combined instruction-record and printer surface is still large. Idea 77
found no blocker and no evidence that status/printer validation should become a
new shared authority route. That means the next cleanup should be a
target-local contraction pass: table-driven naming, shared local validation
helpers, and record/printer helper consolidation where it preserves the record
contract.

## Owned Files

- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.hpp`

## In Scope

- Inventory record-kind name tables, opcode mnemonic tables, prepared-record
  error name tables, status builders, and printer validation helpers.
- Consolidate same-shaped local tables or helpers when the resulting ownership
  is clearer.
- Keep AArch64 register spelling, memory operand spelling, call-boundary
  printing, atomic printing, and frame printing target-local.
- Preserve existing status/printer diagnostics.

## Out Of Scope

- Moving machine-record schema or printer spelling into BIR/prealloc.
- Changing the textual assembly output contract except for intentional,
  reviewed formatting fixes.
- Combining this with dispatch or call lowering relocation.

## Proof Expectations

- Build proof after each contraction slice.
- Backend tests or dump tests covering representative branch, scalar, memory,
  frame, atomic, call-boundary, aggregate-lane, and printer error paths.
- Review specifically checks that diagnostics were not silently weakened.

## Reviewer Reject Signals

- Shared BIR starts owning AArch64 mnemonic spelling or concrete register
  spelling.
- Printer validation is deleted rather than consolidated.
- Instruction record names are changed without corresponding proof.
- The route tries to solve calls, memory, and dispatch layout at the same time.

