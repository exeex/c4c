Status: Active
Source Idea Path: ideas/open/241_inline_asm_clobber_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Carry Clobbers Through BIR And Prepared Records

# Current Packet

## Just Finished

Plan Step 2 - Carry Clobbers Through BIR And Prepared Records.

Added structured inline-asm clobber carriage from parser/HIR through LIR into
BIR and prepared inline-asm carrier records:

- `InlineAsmStmt` now carries parser clobber children in `clobbers` instead of
  relying on appended rendered `~{...}` constraint text as the only fact.
- `LirInlineAsmOp` carries `clobbers`, while HIR-to-LIR still renders
  compatible LLVM `~{name}` constraint spelling from that structured list.
- `InlineAsmMetadata` and `PreparedInlineAsmCarrier` now preserve structured
  `clobbers`; clobber operands are accepted only when the rendered token is
  backed by the structured list.
- Rendered-only `~{...}` tokens remain fail-closed with
  `unsupported_clobber_constraintN` / `unsupported_clobber_operandN` facts.
- Narrow BIR and prepared-printer tests cover supported structured clobber
  carriage plus rendered-only fail-closed behavior.

## Suggested Next

Plan Step 3 packet: add selected AArch64 clobber carriage and target validation
from the prepared structured clobber list, without inferring clobbers from
rendered constraint text or machine-printer output.

## Watchouts

- Do not parse clobbers from rendered template text, final assembly, or
  diagnostic strings.
- Treat source parser clobber children as the authority; rendered `~{...}`
  constraint text is compatibility spelling, not ownership.
- AArch64 selected-machine support is intentionally still absent in this slice;
  the next packet should validate target-supported spellings before selection.
- Preserve the completed inline-asm behavior from closed idea 240.
- Keep memory/address constraints in
  `ideas/open/242_inline_asm_memory_address_constraints.md`.
- Keep alias-aware tied-home allocation policy in
  `ideas/open/243_inline_asm_tied_home_allocation_policy.md`.

## Proof

Delegated proof passed:

`cmake --build --preset default --target backend_lir_to_bir_notes_test backend_prepared_printer_test`

`ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_prepared_printer)$' | tee test_after.log`

Proof log: `test_after.log`.
