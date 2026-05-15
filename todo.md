Status: Active
Source Idea Path: ideas/open/242_inline_asm_memory_address_constraints.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Prepared-Home And Constraint Surfaces

# Current Packet

## Just Finished

Step 1 inventory completed for inline-asm memory/address prepared-home and
constraint surfaces.

Concrete surfaces found:

- BIR inline-asm constraint classification lives in
  `src/backend/bir/lir_to_bir/calling.cpp`:
  `make_inline_asm_metadata` currently recognizes only `r`, `=r`, tied decimal
  operands, `i`/`I`, and `~{...}` clobbers. A memory constraint such as `m`
  records `unsupported_constraintN:m`; there is no memory/address operand kind.
- BIR inline-asm metadata lives in `src/backend/bir/bir.hpp`:
  `InlineAsmOperandKind` has no memory or address variants, and
  `InlineAsmOperandMetadata` has no `MemoryAddress` or prepared-address
  authority field. Generic `bir::MemoryAddress` already exists but is only
  attached to load/store/address-producing instructions, not inline asm
  operands.
- Prepared inline-asm carriers live in `src/backend/prealloc/prealloc.hpp` and
  `src/backend/prealloc/prealloc.cpp`: `PreparedInlineAsmOperand` preserves
  `value`, `value_name`, `home`, and immediate facts only. Validation accepts
  register, tied-register, immediate, and structured clobber facts; memory and
  address constraints fall through as unsupported. Prepared carrier dump
  coverage lives in `tests/backend/bir/backend_prepared_printer_test.cpp`.
- Existing prepared memory/address authority is separate from inline asm:
  `PreparedAddress`, `PreparedMemoryAccess`, `PreparedAddressMaterialization`,
  and `PreparedAddressingFunction` are in `src/backend/prealloc/prealloc.hpp`;
  AArch64 selected memory/address record helpers are in
  `src/backend/mir/aarch64/codegen/instruction.cpp`. These are keyed by normal
  memory/address-producing instructions, not by inline-asm operand
  `constraint_index`.
- AArch64 inline-asm selection lives in
  `src/backend/mir/aarch64/codegen/dispatch.cpp`: `lower_inline_asm_instruction`
  copies prepared inline-asm operands into `InlineAsmMachineOperandRecord` and
  selects only register/tied/immediate operands. Unsupported/clobber operand
  kinds fail closed with diagnostics.
- AArch64 inline-asm printing lives in
  `src/backend/mir/aarch64/codegen/machine_printer.cpp`:
  `inline_asm_constraint_matches_kind` rejects anything except `r`, `=r`, tied
  digits, and `i`/`I`; tests already assert `m` fails as unsupported instead of
  being inferred from template text.
- AArch64 inline-asm selection/printer coverage lives in
  `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` and
  `tests/backend/mir/backend_aarch64_machine_printer_test.cpp`; end-to-end
  supported AArch64 inline asm smoke coverage is currently
  `tests/c/internal/inline_asm/aarch64/simple.c`.

First representative is blocked, not supported yet: a local-object memory input
such as `asm volatile("" : : "m"(x) : "memory")` cannot be accepted
semantically today because no inline-asm operand carries a structured
`MemoryAddress`/prepared-memory record or target-valid AArch64 memory-operand
authority. The analogous address-input representative is blocked for the same
reason: prepared address materialization exists, but inline-asm operands have
no field tying a constraint operand to a target-valid prepared address record.

## Suggested Next

First implementation packet: define the fail-closed structured contract for
memory/address inline-asm operands without selecting or printing support yet.
Add BIR operand kinds for memory/address constraints, carry optional structured
memory/address authority through `InlineAsmOperandMetadata` and
`PreparedInlineAsmOperand`, and update prepared validation/printer tests so
`m` and one address constraint produce precise missing/unsupported prepared
authority diagnostics instead of the generic `unsupported_constraintN:*`.

## Watchouts

- Do not infer memory/address support from rendered assembly text.
- Do not add allocator, spill, scratch-register, clobber ingress, or tied-home
  coallocation policy in this route.
- Keep the first implementation packet fail-closed: it should create structured
  fields and diagnostics, not claim support for `m`/address operands until
  selection can consume complete target-valid homes.
- Reuse or reference the existing prepared memory/address authority surfaces
  where possible, but do not key inline-asm acceptance off normal load/store
  instruction indexes unless the prepared data explicitly relates that record
  to the inline-asm operand `constraint_index`.
- Existing `m` rejection tests in `backend_prepared_printer_test.cpp` and
  `backend_aarch64_machine_printer_test.cpp` should become more precise, not
  weaker. Avoid expectation-only progress.

## Proof

No build required for this inventory-only lifecycle scratchpad update; proof is
not applicable. Suggested narrow proof for the first implementation packet:

`cmake --build build --target backend_prepared_printer_test backend_aarch64_instruction_dispatch_test backend_aarch64_machine_printer_test && ctest --test-dir build -R '^(backend_prepared_printer|backend_aarch64_instruction_dispatch|backend_aarch64_machine_printer)$' --output-on-failure`
