Status: Active
Source Idea Path: ideas/open/242_inline_asm_memory_address_constraints.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Memory/Address Prepared-Home Authority

# Current Packet

## Just Finished

Step 2 implementation completed for the fail-closed memory/address prepared-home
authority contract.

Concrete work completed:

- Added structured BIR inline-asm operand kinds for memory input `m` and address
  input `p`, with optional `MemoryAddress` authority fields on
  `InlineAsmOperandMetadata`.
- Updated LIR-to-BIR inline-asm constraint classification so `m` and `p`
  consume input operands and no longer collapse into generic
  `unsupported_constraintN:*` facts.
- Carried the optional structured memory/address facts into
  `PreparedInlineAsmOperand` and prepared carrier dumps.
- Added fail-closed prepared validation facts for missing authority:
  `missing_operandN_memory_address_authority` and
  `missing_operandN_address_authority`.
- Kept populated memory/address authority fail-closed until selection support is
  implemented with `unsupported_operandN_memory_address_selection` and
  `unsupported_operandN_address_selection`.
- Tightened prepared-printer coverage so the `m` and `p` operands remain
  structured in the prepared carrier while both missing-authority and
  populated-but-not-selectable carriers stay missing.

## Suggested Next

Next packet: Step 3 can decide whether any target-valid prepared memory/address
home is currently expressible for AArch64 inline asm. If yes, route only that
complete structured authority into selection/printing; if not, keep selection
fail-closed and record the exact missing authority bridge.

## Watchouts

- This packet intentionally did not add AArch64 memory/address selection or
  printing support.
- `p` is the structured address constraint representative for this packet.
- Rendered assembly text remains non-authoritative; prepared memory/address
  acceptance must come from structured operand facts.
- The new optional fields are not populated from normal load/store addressing
  records yet, and populated fields are still explicitly not selectable, so
  supported representatives remain blocked until Step 3 bridges complete
  target-valid authority and selection support together.

## Proof

Passed:

`cmake --build --preset default --target backend_prepared_printer_test backend_aarch64_instruction_dispatch_test backend_aarch64_machine_printer_test && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_printer|backend_aarch64_instruction_dispatch|backend_aarch64_machine_printer)$' | tee test_after.log`

Proof log: `test_after.log`
