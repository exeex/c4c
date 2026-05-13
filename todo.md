Status: Active
Source Idea Path: ideas/open/215_aarch64_first_machine_node_selection_slice.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inspect Existing Machine Node Selection Inputs

# Current Packet

## Just Finished

Completed Step 1 inspection for `plan.md` Step 1: Inspect Existing Machine Node
Selection Inputs.

Accepted target-MIR record inputs for the first selection slice:

- Scalar ALU/cast records in `src/backend/mir/aarch64/codegen/records.hpp`:
  `ScalarAluRecord`, `ScalarCastRecord`, `ScalarInstructionRecord`,
  `OperandRecord`, `RegisterOperand`, `ImmediateOperand`, and
  `PreparedValueOperand`. The prepared builders already accept integer
  `Add`, `Sub`, `And`, `Or`, `Xor` and simple integer `SExt`, `ZExt`, `Trunc`
  when result and operands have prepared register/immediate storage, and they
  fail closed for unsupported opcodes, missing homes/storage, unsupported
  storage, unsupported types, and register-view conversion failures.
- Branch/compare records in `codegen/records.hpp`: `BranchInstructionRecord`,
  `BranchTargetOperand`, `BranchTargetPairRecord`, `BranchConditionRecord`,
  `ComparePredicateRecord`, `CompareOperandPairRecord`, and
  `BranchCompareCandidateRecord`. The prepared builders already cover
  unconditional branches, materialized-bool conditional branches, and fused
  integer compare branches with prepared target ids and compare operands; they
  fail closed for missing targets, target mismatches, missing condition homes,
  missing compare facts, and unsupported compare predicates.
- Memory operand records in `codegen/records.hpp`: `MemoryOperand`,
  `MemoryInstructionRecord`, `FrameSlotOperand`, `SymbolOperand`, and
  `RegisterOperand`. Prepared memory operands preserve function/block/index,
  load/store value identity, base kind, frame slot id, symbol id, pointer value
  id/name, string ids, byte offset, size, alignment, address space, volatility,
  and base-plus-offset capability. The first selection should use only
  prepared frame-slot or register/pointer memory operands where all required
  facts are present; global/string/linker-sensitive forms should fail closed in
  this slice.
- Allocation and spill/reload records in `src/backend/mir/aarch64/module/`:
  `TargetRegisterRecord`, `OperandRecord`, `FrameSlotRecord`,
  `MoveRecord`, `AbiBindingRecord`, `ParallelCopyRecord`, and
  `SpillReloadRecord`. The spill/reload records preserve pseudo kind, value id,
  block/instruction index, register bank/class, scratch register authority,
  occupied scratch registers, slot id, prepared stack-offset snapshot, and
  source prepared op pointer. `TargetRegisterRecord` carries typed
  `abi::RegisterReference` when parsing the prepared physical register
  succeeds.

`MACHINE_INSTRUCTION_NODE_CONTRACT.md` requirements the implementation must
publish before selection is accepted:

- family identity plus concrete opcode or family-specific opcode enum; mnemonic
  strings are diagnostics only
- pseudo marker for selected pseudo operations such as spill/reload
- ordered typed operands using register/immediate/memory/symbol/branch-target/
  predicate/prepared-value/frame-slot categories
- provenance: function, block, instruction index, prepared value ids/names,
  source BIR opcode/cast/terminator, and prepared allocation/control/memory
  facts that authorized the node
- def/use/clobber/side-effect metadata sufficient for validation and later
  printer/encoder consumers
- no `.s` printing, assembly parsing, encoding, object writing, or linker
  behavior as the semantic handoff

Machine-node representation finding:

- A generic data-only `InstructionRecord` representation already exists in
  `codegen/records.hpp`, and `make_branch_instruction`,
  `make_scalar_instruction`, `make_memory_instruction`, `make_call_instruction`,
  and `make_return_instruction` mark records as
  `RecordSurfaceKind::MachineInstructionNode`.
- That representation is not yet enough for this plan by itself: it has no
  concrete AArch64 opcode enum, no pseudo marker, no explicit def/use/clobber
  vectors, limited side-effect metadata, no selector result/error surface, and
  no module-owned selected-node list. Step 2 should add a minimal selected-node
  model or tighten the existing one rather than inventing a broad backend
  pipeline.

First safe subset:

- Scalar nodes: integer register/immediate `Add`, `Sub`, `And`, `Or`, `Xor`
  from accepted `ScalarAluRecord` values with GPR register references or
  rematerialized immediate operands; fail closed for compares, multiply/divide,
  floating point, vector, frame-slot operands, missing storage, or missing typed
  register conversion.
- Cast nodes: simple integer `SExt`, `ZExt`, and `Trunc` from accepted
  `ScalarCastRecord` values with prepared register source/result facts; fail
  closed for FP casts, pointer/int casts that lack explicit accepted semantics,
  unsupported types, and non-register result storage.
- Branch nodes: unconditional branch, materialized-bool conditional branch, and
  fused integer compare branch from accepted branch records; preserve target
  labels, condition value id/name, predicate, compare operands, and fusion
  candidate metadata. Fail closed for unsupported predicates, missing compare
  homes, target mismatches, calls/returns, switch-like forms, and any
  fallthrough policy not directly represented by target-MIR records.
- Spill/reload pseudo nodes: selected pseudo store-to-slot and reload-to-scratch
  from `SpillReloadRecord`, using `TargetRegisterRecord` scratch authority,
  typed register reference, slot id, prepared offset snapshot, bank/class,
  occupied registers, value id, block/index, and source prepared op. Do not
  compute stack offsets or choose final `str`/`ldr` forms locally.
- Memory nodes: only prepared load/store node construction over already
  accepted `MemoryOperand` facts where the address base and value identity are
  complete. Keep global, string, TLS, linker, call-frame, outgoing-area,
  prologue/epilogue, and final frame-materialization forms deferred unless
  their facts are already explicit and the selector can report unsupported
  status cleanly.

Chosen code/test surfaces for the next packet:

- Code: start under `src/backend/mir/aarch64/codegen/records.hpp` and
  `records.cpp` for the minimal node/effect/diagnostic surface. Add only the
  smallest module boundary hook in `src/backend/mir/aarch64/module/` if Step 2
  needs a module-owned selected-node list.
- Tests: extend focused structured-record tests, especially
  `backend_aarch64_target_instruction_records`,
  `backend_aarch64_scalar_record_contract`,
  `backend_aarch64_prepared_scalar_alu_records`,
  `backend_aarch64_prepared_scalar_cast_records`,
  `backend_aarch64_prepared_branch_records`,
  `backend_aarch64_memory_operand_records`,
  `backend_aarch64_prepared_memory_operand_records`, and
  `backend_aarch64_prepared_frame_control`. These tests already inspect
  structured records directly without assembly text.

Unsupported records that should fail closed:

- call, return, ABI binding, prologue/epilogue, outgoing call area, variadic,
  FP/vector/atomics/intrinsics/inline-asm, globals/string/linker relocation,
  object/encoder, assembler external-input, unresolved virtual registers,
  non-prepared memory, malformed physical registers, and spill/reload records
  without slot id or typed scratch authority.

Missing-carrier findings:

- Need concrete opcode or opcode-family enum for selected scalar, branch,
  memory, and spill/reload pseudo nodes.
- Need explicit node effect metadata: defs, uses, clobbers, and side effects.
- Need selector result/status diagnostics so unsupported records fail closed
  without silent skipping.
- Need a clear ownership point for selected nodes. Existing `codegen`
  instruction records can be reused or tightened, but the built module does not
  yet publish selected machine nodes.
- Need a typed bridge from module spill/reload records to node operands that
  uses `TargetRegisterRecord::register_reference` and fails closed if it is
  absent.

## Suggested Next

Implement Step 2 by adding the smallest selected-node/effect/status surface for
the safe subset above, preferably by tightening `codegen/records.hpp` and
`records.cpp` instead of creating a parallel representation unless the existing
`InstructionRecord` shape cannot carry the required contract fields cleanly.

## Watchouts

- Do not emit or parse assembly text under this plan.
- Select only from accepted target-MIR records; do not bypass through rendered
  BIR, old examples, or fixture names.
- Keep the first subset small and fail closed for unsupported records.
- Consume allocation, scratch, spill slot, memory operand, and provenance facts
  from accepted records instead of recomputing them locally.
- Existing `InstructionRecord` has `MachineInstructionNode` surface naming but
  lacks opcode/effect/status fields; do not treat the name alone as completion
  of the machine-node contract.
- Keep call/return/prologue/variadic/global/linker behavior out of the first
  selection subset even though placeholder record variants exist.

## Proof

Inspection-only packet. No build required by delegated proof. Ran targeted
`c4c-clang-tool` function-signature inspection over AArch64 record headers plus
raw reads of the machine-node contract, target-MIR/module record surfaces, and
focused tests. Required validation: `git diff --check`.
