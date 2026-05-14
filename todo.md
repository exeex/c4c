Status: Active
Source Idea Path: ideas/open/224_common_mir_container_and_target_printer_boundary.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Current MIR Carrier And Printer Boundary

# Current Packet

## Just Finished

Step 1 - Inventory Current MIR Carrier And Printer Boundary completed as an
inventory-only packet. No implementation files were changed.

Direct flat-vector or flattening callers found:

- `src/backend/backend.cpp:181`: public AArch64 assembly generation calls
  `c4c::backend::mir::flatten_instructions(function.mir)`, copies
  `MachineNode<InstructionRecord>::target` into a temporary
  `std::vector<InstructionRecord>`, then calls
  `print_machine_instruction_nodes(...)`.
- `src/backend/mir/aarch64/module/module.cpp:1831`: `build_mir_function(...)`
  takes `const std::vector<codegen::InstructionRecord>& machine_nodes` as the
  source of truth and appends those records into the common MIR function.
- `src/backend/mir/aarch64/module/module.cpp:2057`: `build_function_record(...)`
  builds and appends the compatibility `record.machine_nodes` flat vector,
  then derives `record.mir` from that vector at line 2079.
- Tests with direct `function.machine_nodes` reads: `backend_aarch64_prepared_frame_control_test.cpp`,
  `backend_aarch64_prepared_scalar_alu_records_test.cpp`, and
  `backend_aarch64_prepared_module_identity_test.cpp`. These are metadata
  contract readers, not public assembly printers, and should stay until the
  common-MIR path has replacement assertions.
- `src/backend/mir/mir.hpp` helper callers are otherwise narrow:
  `mir::empty(function.mir)` in `backend.cpp` and `mir::append_instruction(...)`
  in AArch64 module construction.

Display-cache and spelling candidates:

- AArch64 target module records cache many display spellings in
  `src/backend/mir/aarch64/module/module.hpp`, including function/block labels,
  register names, occupied register names, symbol labels, stack slot/value
  labels, global/string labels, and data relocation labels. Most are metadata
  and diagnostic display fields rather than the current public assembly path.
- Public instruction spelling is concentrated in
  `src/backend/mir/aarch64/codegen/machine_printer.cpp`: block labels,
  register names, immediates, memory addresses, mnemonics, and indentation are
  currently emitted there.
- Mnemonic spelling is derived through `MachinePrinterMnemonicKind` helpers in
  `src/backend/mir/aarch64/codegen/records.cpp`, not cached per instruction.
- Register display names are recoverable from structured register references
  through `register_display_name(...)` and `abi::register_name(...)`; legacy
  prepared register-name fallbacks still exist in AArch64 module construction
  and should not be removed until replacement assertions prove typed placement
  authority remains intact.
- `InstructionRecord::selection.diagnostic` is diagnostic text only and is not
  a printer input for successful assembly.

Minimum common printer/walker surface for current public AArch64 assembly:

- Walk `Function<MachineNode<TargetInstruction>>` in function block order,
  skipping empty functions consistently with the current route.
- Own common structure formatting for `.text`, `.globl`, `.type`, function
  label, `.size`, final `.note.GNU-stack`, per-instruction indentation/newline
  policy, and block/instruction traversal.
- Delegate all target instruction spelling to a target callback such as
  `print_instruction(const TargetInstruction&) -> MachineAssemblyPrintResult`.
- Delegate target-owned label spelling because current AArch64 block labels are
  `.LBB<function_id>_<block_label_id>` and common MIR must not know that
  target syntax.
- Keep diagnostics able to report function/block/instruction position when a
  target instruction printer fails.

## Suggested Next

First implementation packet: add the common MIR function walker/printer surface
under `src/backend/mir/` without changing public AArch64 assembly routing yet.
Keep it target-neutral and behavior-preserving by delegating target instruction
and label spelling to callbacks; prove it with a focused common/AArch64 printer
test before routing `backend.cpp` through it.

Suggested exact proof command for that first implementation packet:

```sh
cmake --build build --target c4c_backend c4cll backend_aarch64_machine_printer_test && ctest --test-dir build -R '^(backend_aarch64_machine_printer|backend_cli_aarch64_asm_no_machine_nodes_fails)$' --output-on-failure | tee test_after.log
```

## Watchouts

- Common MIR must own traversal and formatting structure, not AArch64 spelling.
- Do not add unrelated AArch64 instruction coverage to make the migration look
  larger than it is.
- Do not weaken public assembly or MIR tests while moving the printer boundary.
- Keep prepared authority facts semantically unchanged.
- The existing public route still depends on `function.label` for function
  symbol spelling and AArch64 printer-local `.LBB...` spelling for branch
  targets; the first common printer surface needs callback hooks for both
  rather than hard-coded common syntax.
- Do not remove `machine_nodes` in the first implementation packet. It remains
  the construction source and is still read by tests that assert prepared
  metadata contracts.

## Proof

No build/test required for this Step 1 inventory-only todo update because no
implementation behavior changed. No `test_after.log` was produced for this
packet.
