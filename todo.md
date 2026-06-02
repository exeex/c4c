# Current Packet

Status: Active
Source Idea Path: ideas/open/87_aarch64_call_boundary_record_printer_surface_audit.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Call-Boundary Record Surfaces

## Just Finished

Completed Step 1, "Inventory Call-Boundary Record Surfaces", by tracing the
call-boundary record path from construction through instruction records and
printer dispatch.

### Step 1 Call-Boundary Record Inventory

- `src/backend/mir/aarch64/codegen/calls.cpp`:
  `lower_prepared_call_instruction` builds `CallInstructionRecord` with direct
  or indirect callee, prepared argument/result plans, memory-return storage,
  clobber/preserve facts, outgoing stack bytes, variadic helper facts, and call
  flags; `make_call_instruction` turns it into a selected
  `InstructionFamily::Call` node.
- `src/backend/mir/aarch64/codegen/calls.cpp`:
  `lower_before_call_move` is the primary call-argument boundary constructor;
  it classifies prepared moves, fills `CallBoundaryMoveInstructionRecord`
  provenance from the move bundle, selects source register, source memory,
  source immediate, f128 carrier facts, and destination register, then emits
  `make_call_boundary_move_instruction`.
- `src/backend/mir/aarch64/codegen/calls.cpp`:
  stack call arguments that target ABI stack slots do not stay in
  `CallBoundaryMoveInstructionRecord`; selected frame-slot-to-stack cases lower
  directly to `MemoryInstructionRecord` stores, while f128 stack arguments can
  lower to f128 transport records.
- `src/backend/mir/aarch64/codegen/calls.cpp`:
  immediate call-argument publication constructs a synthetic
  `PreparedMoveResolution` with reason `call_arg_immediate_to_register`, stores
  the literal in `source_immediate`, and emits a call-boundary move.
- `src/backend/mir/aarch64/codegen/calls.cpp`:
  `lower_after_call_move` is the call-result boundary constructor; selected
  register results populate source/destination register operands and f128
  carrier facts, but stack result publication lowers directly to
  `MemoryInstructionRecord` stores.
- `src/backend/mir/aarch64/codegen/calls.cpp`:
  before-return ABI moves are represented through the same
  `CallBoundaryMoveInstructionRecord` schema with `BeforeReturn` phase and
  `FunctionReturnAbi` destination facts.
- `src/backend/mir/aarch64/codegen/calls.cpp`:
  scratch/preservation surfaces build synthetic value moves for callee-saved
  preservation home population and republication; they reuse
  `CallBoundaryMoveInstructionRecord` with `Value` destinations and either
  source registers or prepared frame-slot source memory.
- `src/backend/mir/aarch64/codegen/calls.cpp`:
  block-entry and before-instruction value moves reuse the call-boundary move
  record for prepared value publication, including register, immediate, and
  prepared frame-slot reload sources.
- `src/backend/mir/aarch64/codegen/calls.cpp`:
  `make_call_boundary_move_instruction` owns the conversion from move record to
  `InstructionRecord` operands, defs, uses, family
  `InstructionFamily::CallBoundary`, surface
  `RecordSurfaceKind::MachineInstructionNode`, opcode
  `MachineOpcode::CallBoundaryMove`, and selection status.
- `src/backend/mir/aarch64/codegen/calls.cpp`:
  `make_call_boundary_abi_binding_instruction` carries prepared ABI binding
  provenance as a `CallBoundaryAbiBindingInstructionRecord` with call-boundary
  family/opcode; the printer marks it unsupported until later move lowering.
- `src/backend/mir/aarch64/codegen/calls.cpp`:
  `call_boundary_move_selection_status` validates selected register argument,
  result, return, value, preservation, immediate, frame-slot, aggregate-lane,
  scalar FPR, and f128 q-register subsets; `call_boundary_abi_binding_selection_status`
  only requires bundle/binding/function provenance.
- `src/backend/mir/aarch64/codegen/calls.hpp`:
  public call-boundary APIs expose lowering entry points plus record consumer
  helpers that retarget source memory to emitted scalar registers, record
  destinations into scalar state, identify prepared stack-source reloads, and
  reason about materialized-address conflicts.
- `src/backend/mir/aarch64/codegen/instruction.hpp`:
  `CallBoundaryMoveInstructionRecord` carries function/phase/authority/block
  provenance, optional parallel-copy labels, the prepared move, optional
  immediate/register/memory sources, `source_memory_materializes_address`,
  destination register, f128 constant/carrier facts, and source bundle/move
  pointers.
- `src/backend/mir/aarch64/codegen/instruction.hpp`:
  `CallBoundaryAbiBindingInstructionRecord` carries function/phase/authority
  provenance, binding index, optional parallel-copy labels, the prepared ABI
  binding, and source bundle/binding pointers.
- `src/backend/mir/aarch64/codegen/instruction.hpp`:
  `CallInstructionRecord` is the call-site schema, not the boundary-move
  schema; it owns callee operands, argument/result operand records, prepared
  plans, memory-return facts, clobbers/preserves, variadic facts, convention,
  and indirect/variadic/noreturn flags.
- `src/backend/mir/aarch64/codegen/instruction.hpp` and
  `src/backend/mir/aarch64/codegen/instruction.cpp`:
  `RecordSurfaceKind`, `InstructionFamily`, `MachineOpcode`, spelling helpers,
  and surface classifier helpers name the structured node surface used by
  call-boundary records.
- `src/backend/mir/aarch64/codegen/instruction.cpp`:
  `is_aggregate_register_lane_publication` is a call-boundary move schema
  predicate for byval aggregate register-lane moves; it is relevant to Step 2
  but already confirms that aggregate-lane printing rides on
  `CallBoundaryMoveInstructionRecord`.
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`:
  `print_machine_instruction_line_payloads` dispatches
  `CallBoundaryMoveInstructionRecord` to `print_call_boundary_move`,
  `CallBoundaryAbiBindingInstructionRecord` to
  `print_call_boundary_abi_binding`, and `CallInstructionRecord` to
  `print_call` after generic selected-node validation.
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`:
  `print_call_boundary_move` owns assembly spelling and printer-side validation
  for provenance, register/immediate/memory presence, aggregate-lane
  publication, frame-slot loads/address materialization, scalar integer and FP
  immediate moves, f128 q-register moves, scalar FPR moves, and ordinary moves.
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`:
  call-boundary scratch printer helpers choose/address scratch registers and
  materialize non-encodable frame-slot addresses; these duplicate some
  encodability checks from construction but own final printable assembly
  constraints.
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`:
  `print_call` owns direct/indirect call spelling and variadic call wrappers,
  while `print_call_boundary_abi_binding` is an explicit unsupported sentinel
  for binding records that should have been converted into concrete moves.
- `src/backend/mir/aarch64/codegen/machine_printer.hpp`:
  public printer surface is narrow: `MachineInstructionPrinter::print_instruction`
  and `print_machine_instruction_line_payloads`.

## Suggested Next

Execute Step 2, "Inventory Aggregate-Lane Publication Surfaces", by tracing
the byval aggregate register-lane path that uses
`CallBoundaryMoveInstructionRecord` and separating shared call-boundary
ownership from aggregate-specific printing helpers.

## Watchouts

- This is audit-only; do not edit implementation files while executing the
  current plan.
- Do not claim printer cleanup by deleting validation or moving ABI-specific
  call-boundary construction into shared authority.
- Treat unclear duplication as `missing-evidence` until the record path and
  owner boundary are traced.
- Step 1 found that stack argument/result publications often bypass
  `CallBoundaryMoveInstructionRecord` and lower directly to memory or transport
  records; follow-up classification should not describe those as missing
  call-boundary printer coverage.
- Aggregate-lane publication is already visible as a special predicate and
  printer branch on `CallBoundaryMoveInstructionRecord`; Step 2 should decide
  whether that is shared schema ownership or an aggregate-specific surface.

## Proof

Audit-only proof command:

```sh
printf 'Audit-only Step 1; no backend tests required.\n' > test_after.log && git diff --name-only >> test_after.log && if git diff --name-only | rg -q '^src/backend/mir/aarch64/codegen/(calls|instruction|machine_printer)\.(cpp|hpp)$|^plan\.md$|^ideas/'; then printf 'ERROR: non-todo file changed during audit-only call-boundary inventory packet.\n' >> test_after.log; exit 1; fi
```

Result: passed; `test_after.log` contains the audit-only proof marker and dirty
file list.
