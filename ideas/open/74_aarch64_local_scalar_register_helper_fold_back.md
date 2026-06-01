# AArch64 Local Scalar Register Helper Fold-Back

## Goal

Inventory and fold repeated AArch64-local scalar type, register-view, and
compare predicate helper logic into one focused local helper surface without
moving target instruction selection or printer spelling into shared code.

## Why This Exists

The large-owner residue audit classified repeated scalar type, register-view,
and compare predicate helper logic across printer, instruction, ALU, and
comparison owners as `fold-back-or-split`. The better boundary is a narrow
AArch64-local helper surface, likely near instruction construction or
ABI/register-profile helpers, with BIR type facts and
`abi::convert_prepared_register` as inputs.

## Owned Files

- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/comparison.cpp`
- Any new AArch64-local helper file created beside these owners.

## In Scope

- First inventory exact duplicate scalar type, register-view, and compare
  predicate helpers.
- Fold true duplicates into one AArch64-local helper boundary.
- Keep BIR facts and `abi::convert_prepared_register` as inputs rather than
  re-deriving shared semantics.
- Preserve target-local record construction, opcode selection, condition-code
  spelling, and printer text formatting.

## Out Of Scope

- Changing shared BIR, prealloc, or ABI authority to absorb AArch64-local
  register-view spelling.
- Splitting `instruction.hpp` by schema family unless duplicate helper removal
  makes that locally necessary.
- Rewriting machine printer formatting or instruction record schemas for
  line-count reduction.
- Touching call, memory, special-carrier, or variadic cleanup.

## Proof Expectations

- Focused build and tests covering scalar ALU, comparisons, instruction record
  construction, and machine-printer output for the helper families touched.
- A before/after inventory note showing which duplicate helpers were removed
  or unified.
- Regression guard logs for acceptance-sized slices.

## Reviewer Reject Signals

- The route claims capability progress through helper moves only, without
  removing real duplicate local helper logic.
- A shared BIR or prealloc helper starts owning AArch64 register-view,
  condition-code, record-schema, or printer-spelling details.
- The route changes printed assembly, record construction, or compare semantics
  without targeted proof.
- Broad instruction schema or printer rewrites are mixed into this local
  fold-back.
- The exact old duplication survives behind a new abstraction name.
