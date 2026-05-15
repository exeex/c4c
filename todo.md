Status: Active
Source Idea Path: ideas/open/242_inline_asm_memory_address_constraints.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Bridge Prepared Authority Into Selected Memory/Address Operands

# Current Packet

## Just Finished

Step 3 completed as a fail-closed selection/printer boundary for inline-asm
memory/address operands.

Complete target-valid selection is not expressible yet inside the owned selected
machine surface: prepared carriers can retain optional structured
`MemoryAddress` authority, but `InlineAsmMachineOperandRecord` has no
structured memory/address payload slot and there is no prepared-inline-asm
bridge that converts that authority into a target-valid selected AArch64 memory
or address operand without inventing allocator/scratch policy.

Concrete work completed:

- Added explicit AArch64 dispatch diagnostics for complete-but-not-selectable
  prepared `MemoryInput` and `AddressInput` operands:
  `unsupported_inline_asm_memory_address_selection` and
  `unsupported_inline_asm_address_selection`.
- Added explicit selected-printer diagnostics for selected records that contain
  memory/address inline-asm operand kinds without selected structured authority.
- Added dispatch and printer tests proving these operands remain fail-closed at
  the selected-machine boundary instead of being inferred from rendered template
  text.

## Suggested Next

Lifecycle decision: keep `ideas/open/242_inline_asm_memory_address_constraints.md`
active and rewrite the runbook in place. The selected-machine authority bridge
is required current-scope work, not a separate idea, because the source idea
already requires AArch64 selection to accept complete target-valid structured
memory/address operands when expressible.

Next packet: Step 4, add the missing selected-machine authority bridge only
when the design can carry a structured prepared `MemoryAddress` into a
target-valid AArch64 inline-asm memory/address operand record and printer form
without using rendered text or adding allocator/scratch/spill policy.

## Watchouts

- Normal preparation still marks populated memory/address authority as missing
  with `unsupported_operandN_memory_address_selection` and
  `unsupported_operandN_address_selection`; this packet only hardens the
  selected-machine boundary if such operands reach AArch64 dispatch/printer.
- Do not infer memory/address operands from inline-asm template text or final
  assembly text.
- The missing bridge is structural: selected inline-asm machine operands need a
  target-valid authority carrier for `MemoryAddress` and a printer form before
  any `m`/`p` operand can be accepted.

## Proof

Passed:

`cmake --build --preset default --target backend_prepared_printer_test backend_aarch64_instruction_dispatch_test backend_aarch64_machine_printer_test && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_printer|backend_aarch64_instruction_dispatch|backend_aarch64_machine_printer)$' | tee test_after.log`

Proof log: `test_after.log`
