Status: Active
Source Idea Path: ideas/open/242_inline_asm_memory_address_constraints.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Bridge Prepared Authority Into Selected Memory/Address Operands

# Current Packet

## Just Finished

Step 4 completed the first selected-machine authority bridge for prepared
inline-asm memory/address operands.

Concrete work completed:

- Prepared inline-asm validation now treats pointer-value `MemoryAddress`
  authority with a concrete prepared register home as selectable instead of
  recording unsupported memory/address selection facts.
- AArch64 inline-asm dispatch converts that prepared pointer-address authority
  into a selected structured `MemoryOperand` carried in the existing selected
  operand slot, without adding allocator, scratch, spill, rendered-template, or
  final-assembly inference policy.
- AArch64 inline-asm printing accepts `m`/`p` records only when a selected
  structured memory operand is present and still fails closed for records
  without selected authority.
- Added prepared-printer, dispatch, and machine-printer coverage proving a
  representative prepared pointer memory operand reaches selected form and
  prints as `[x5, #8]`.

## Suggested Next

Next packet: decide whether Step 4 should broaden the bridge beyond
pointer-value base+offset representatives, such as address-input-specific
printing or other target-valid `MemoryAddress` base kinds, while keeping
unsupported non-selected records fail-closed.

## Watchouts

- Do not infer memory/address operands from inline-asm template text or final
  assembly text.
- The supported selected bridge is intentionally limited to structured
  pointer-value addresses whose base value matches the operand value and whose
  prepared home is a concrete GPR register.
- Non-pointer or otherwise unselectable memory/address operands still record
  `unsupported_operandN_memory_address_selection` or
  `unsupported_operandN_address_selection` during preparation.

## Proof

Passed:

`cmake --build --preset default --target backend_prepared_printer_test backend_aarch64_instruction_dispatch_test backend_aarch64_machine_printer_test backend_aarch64_prepared_memory_operand_records_test && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_printer|backend_aarch64_instruction_dispatch|backend_aarch64_machine_printer|backend_aarch64_prepared_memory_operand_records)$' | tee test_after.log`

Proof log: `test_after.log`
