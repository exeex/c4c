Status: Active
Source Idea Path: ideas/open/77_aarch64_machine_status_printer_validation_probe.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Status And Printer Record Families

# Current Packet

## Just Finished

Completed `plan.md` Step 1 evidence inventory for AArch64 machine status
derivation and machine-printer validation ownership.

Status derivation helpers in
`src/backend/mir/aarch64/codegen/instruction.cpp`:

- `branch_selection_status(const BranchInstructionRecord&, const std::vector<OperandRecord>&)`:
  inspects `BranchInstructionRecord`, `BranchConditionRecord`,
  `BranchCompareCandidateRecord`, `ComparePredicateRecord`,
  `CompareOperandPairRecord`, and branch target facts.
- `scalar_selection_status(const ScalarInstructionRecord&)`: inspects
  `ScalarInstructionRecord`, `ScalarAluRecord`, `ScalarUnaryRecord`, and
  `ScalarCastRecord`.
- `address_materialization_selection_status(const AddressMaterializationRecord&)`:
  inspects `AddressMaterializationRecord`.
- `spill_reload_selection_status(const SpillReloadInstructionRecord&)`:
  inspects `SpillReloadInstructionRecord` and its `MemoryOperand` slot/scratch
  facts.
- `frame_selection_status(const FrameInstructionRecord&)`: inspects
  `FrameInstructionRecord` and optional `CalleeSaveInstructionRecord`.

Status-related public constructors declared in
`src/backend/mir/aarch64/codegen/instruction.hpp` and defined in
`instruction.cpp`:

- `make_branch_instruction(BranchInstructionRecord)` applies
  `branch_selection_status`.
- `make_scalar_instruction(ScalarInstructionRecord)` applies
  `scalar_selection_status`.
- `make_address_materialization_instruction(AddressMaterializationRecord)`
  applies `address_materialization_selection_status`.
- `make_spill_reload_instruction(SpillReloadInstructionRecord)` applies
  `spill_reload_selection_status`.
- `make_frame_instruction(FrameInstructionRecord)` applies
  `frame_selection_status`.
- `make_assembler_instruction(AssemblerInstructionRecord)` and
  `make_object_instruction(ObjectInstructionRecord)` assign fixed
  `DeferredUnsupported` status for non-selected downstream surfaces.
- `make_unsupported_machine_instruction(...)` assigns caller-provided status.

Machine-printer validation/check entry points in
`src/backend/mir/aarch64/codegen/machine_printer.cpp`:

- `validate_selected_machine_node(const InstructionRecord&)` checks every
  printable record for `RecordSurfaceKind::MachineInstructionNode` and
  `MachineNodeSelectionStatus::Selected`.
- `print_machine_instruction_line_payloads(const InstructionRecord&)`
  dispatches by payload to printer checks for `SpillReloadInstructionRecord`,
  `BranchInstructionRecord`, `MemoryInstructionRecord`,
  `AtomicMemoryInstructionRecord`, `FrameInstructionRecord`,
  `CallBoundaryMoveInstructionRecord`,
  `CallBoundaryAbiBindingInstructionRecord`, `CallInstructionRecord`,
  `AddressMaterializationRecord`, `I128TransportRecord`, `F128TransportRecord`,
  `I128PairOperationRecord`, `I128ShiftRecord`, `I128CompareRecord`,
  `I128RuntimeHelperBoundaryRecord`, `F128RuntimeHelperBoundaryRecord`,
  `ScalarInstructionRecord`, intrinsic-family records,
  `AssemblerInstructionRecord`, and `ReturnInstructionRecord`.
- Local printer checks in this file include `print_spill_reload`,
  `print_branch`, `print_fused_compare_branch`, `print_memory`,
  `print_symbol_memory`, `print_atomic_memory`, `print_frame`,
  `print_scalar`, `print_return`, `print_call_boundary_move`, `print_call`,
  `print_call_boundary_abi_binding`, and `print_assembler`.
- Printer checks delegated through included helpers from this dispatch include
  `print_address_materialization_instruction`, `print_i128_transport`,
  `print_f128_transport`, `print_i128_pair_operation`, `print_i128_shift`,
  `print_i128_compare`, `print_i128_runtime_helper`,
  `print_f128_runtime_helper`, and `print_intrinsic_instruction`.

Record families present in both status derivation and printer validation:

- Branch: status helper `branch_selection_status`; printer checks
  `print_branch` and fused path `print_fused_compare_branch`.
- Scalar: status helper `scalar_selection_status`; printer check
  `print_scalar` plus delegated ALU/cast print helpers.
- Address materialization: status helper
  `address_materialization_selection_status`; printer dispatch to
  `print_address_materialization_instruction`.
- Spill/reload: status helper `spill_reload_selection_status`; printer check
  `print_spill_reload`.
- Frame: status helper `frame_selection_status`; printer check `print_frame`.

Printer-only record families in `machine_printer.cpp` dispatch:

- `MemoryInstructionRecord`, `AtomicMemoryInstructionRecord`,
  `CallBoundaryMoveInstructionRecord`,
  `CallBoundaryAbiBindingInstructionRecord`, `CallInstructionRecord`,
  `ReturnInstructionRecord`, `AssemblerInstructionRecord`,
  `I128TransportRecord`, `F128TransportRecord`,
  `I128PairOperationRecord`, `I128ShiftRecord`, `I128CompareRecord`,
  `I128RuntimeHelperBoundaryRecord`, `F128RuntimeHelperBoundaryRecord`, and
  intrinsic-family records such as `ScalarFpUnaryIntrinsicRecord`,
  `Crc32WIntrinsicRecord`, `VectorLoadIntrinsicRecord`, and
  `VectorAddIntrinsicRecord`.

Status-only families in `instruction.cpp` status derivation:

- No record family with a dedicated `*_selection_status` helper lacks a
  corresponding printer dispatch/check in `machine_printer.cpp`. The only
  status assignments without paired printer validation are the fixed
  `DeferredUnsupported` constructors for `AssemblerInstructionRecord` and
  `ObjectInstructionRecord`, plus caller-provided status from
  `make_unsupported_machine_instruction(...)`; `AssemblerInstructionRecord` is
  still printer-dispatched, while `ObjectInstructionRecord` is not.

## Suggested Next

Use the paired-family inventory to choose the first targeted semantic
validation fold candidate, keeping final assembly spelling checks in the
printer.

## Watchouts

- Do not change implementation files during the evidence-only inventory.
- Do not fold printer checks until repeated semantic validation is proven.
- Keep final assembly spelling validation in the printer.
- `MemoryInstructionRecord` has prepared-record builders in `instruction.hpp`,
  but no dedicated `*_selection_status` helper in `instruction.cpp`; the printer
  has substantial load/store validation in `print_memory` and
  `print_symbol_memory`.
- `AtomicMemoryInstructionRecord`, call-boundary, call, return, intrinsic,
  i128/f128, and assembler families are printer-dispatched without a matching
  local status helper in `instruction.cpp`.

## Proof

No build required by delegated proof; evidence-only `todo.md` update. No
`test_after.log` was produced.
