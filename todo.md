Status: Active
Source Idea Path: ideas/open/84_aarch64_prepared_consumer_wrapper_contraction.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Build The Prepared Consumer Audit

# Current Packet

## Just Finished

Step 1 - Build The Prepared Consumer Audit completed an inventory pass over
the prepared call-boundary, memory-access, scalar ALU, and scalar/control-flow
consumer helpers. No implementation files were changed.

### Prepared Consumer Audit

| Consumer group / helper | Owner file | Prepared fact inputs | Output record / emission path | Classification | Proof needed before contraction |
| --- | --- | --- | --- | --- | --- |
| `require_prepared_call_plan` / `find_prepared_call_plan` | `src/backend/mir/aarch64/codegen/calls.cpp`, `calls.hpp` | `context.function.call_plan_lookups`, instruction index | Diagnostic-gated `PreparedCallPlan` pointer for call lowering | necessary prepared adapter | Build plus call-boundary tests if signatures change; otherwise retain. |
| `lower_prepared_call_instruction` | `src/backend/mir/aarch64/codegen/calls.cpp`, `calls.hpp` | `PreparedCallPlan`, variadic entry/helper homes, retained BIR call | `CallInstructionRecord` -> `InstructionRecord` -> `MachineInstruction` | target-local emission | Build plus `ctest --test-dir build -R "backend_aarch64_call_boundary_owner|backend_codegen_route_aarch64_prepared_call_boundary_scalability|backend_aarch64_target_instruction_records" --output-on-failure`; retain ABI/register/direct-vs-indirect logic. |
| `find_prepared_frame_slot_call_argument_move` | `src/backend/mir/aarch64/codegen/calls.cpp` | `PreparedCallPlan`, argument plan, move bundle lookups/classification | `PreparedFrameSlotCallArgumentMove` consumed by stack/frame-slot argument publication | necessary prepared adapter | Call-boundary route tests plus prealloc call-boundary classification tests if changed. |
| `lower_scalar_call_argument_producers` / `materialize_scalar_call_argument_value` | `src/backend/mir/aarch64/codegen/calls.cpp` | prepared edge publication source producers, call argument plan, value homes | extra producer `MachineInstruction`s before call | target-local emission | Call-boundary scalability and indirect-call route tests; retain recursive materialization and emitted-scalar state handling. |
| `prepared_call_boundary_source_value` / `materialize_call_boundary_source_to_destination` | `src/backend/mir/aarch64/codegen/calls.cpp` | prepared edge publication source producer lookup, prepared move source memory | assembler materialization to destination register | target-local emission | Call-boundary route tests plus unpublished-load-local call-boundary route. |
| `find_prepared_indirect_callee_source_producer` and stored/select-chain helpers | `src/backend/mir/aarch64/codegen/calls.cpp` | prepared edge producers, prepared names, addressing, same-block stored-value source | producer/dependency facts for indirect callee materialization | necessary prepared adapter | Indirect-call route test plus call-boundary owner tests before any signature fold. |
| `materialize_indirect_call_callee_to_prepared_register` | `src/backend/mir/aarch64/codegen/calls.cpp`, `calls.hpp` | `PreparedCallPlan::indirect_callee`, source producers, prepared value home/register name | select-chain assembler `MachineInstruction`; records emitted scalar register | target-local emission | `ctest --test-dir build -R "backend_codegen_route_aarch64_global_function_pointer_table_selected_indirect_call|backend_aarch64_call_boundary_owner" --output-on-failure`; retain scratch selection and csel emission. |
| `prepared_memory_access` / `prepared_memory_access_matches_instruction` | `src/backend/mir/aarch64/codegen/memory.cpp`, `memory.hpp` | prepared addressing for function/block/instruction | `PreparedMemoryAccess` pointer / retained-BIR match gate | necessary prepared adapter | Memory operand record and contract tests if signatures change. |
| `make_prepared_frame_slot_memory_operand`, `prepared_frame_slot_load_address`, `prepared_local_load_offset` | `src/backend/mir/aarch64/codegen/memory.cpp`, `memory.hpp` | prepared stack layout/addressing/memory access | `MemoryOperand` or concrete frame-slot address/offset | target-local emission | Memory operand record/contract tests; retain frame-pointer/base-plus-offset legality. |
| `make_prepared_memory_operand_record` overloads for load/store local/global | `src/backend/mir/aarch64/codegen/memory.cpp`, `memory.hpp` | prepared names, value locations, addressing, retained BIR memory instruction | `PreparedMemoryOperandRecordResult` | necessary prepared adapter | `backend_aarch64_prepared_memory_operand_records`, `backend_aarch64_memory_operand_records`, memory contract tests before folding shared validation. |
| `make_prepared_load_memory_instruction_record` / `make_prepared_store_memory_instruction_record` overloads | `src/backend/mir/aarch64/codegen/memory.cpp`, `memory.hpp` | prepared operand record, value locations, storage plan, edge publications | `PreparedMemoryInstructionRecordResult` | necessary prepared adapter | Same memory record tests plus route tests for GOT/global and stack publication; retain typed stack source handling. |
| `make_prepared_frame_slot_load_memory_instruction_record` | `src/backend/mir/aarch64/codegen/memory.cpp`, `memory.hpp` | same as prepared load memory record, no edge publications | delegates directly to `make_prepared_load_memory_instruction_record(..., nullptr, ...)` | redundant wrapper | Focused memory record tests; candidate exists but lower priority than ALU because memory has more overload traffic. |
| `plan_store_local_source_publication` and related store-publication planners | `src/backend/mir/aarch64/codegen/memory.cpp` | prepared memory access, stack layout, value homes, source producers, direct-global dependency | `PreparedStoreSourcePublicationPlan` | necessary prepared adapter | Store-source publication and AArch64 store-global/stack-publication route tests; keep prepared planning in prealloc and target emission local. |
| `lower_store_local_value_publication`, pointer writeback, fixed-formal/store-global publication lowerers | `src/backend/mir/aarch64/codegen/memory.cpp`, `memory.hpp` | prepared store-source plan, addressing, value homes, source producers | assembler or memory `MachineInstruction` publication | target-local emission | Store-source publication plus affected AArch64 route tests; retain scratch/addressing/instruction spelling. |
| `make_prepared_scalar_operand`, `make_prepared_scalar_result_register_operand`, `make_prepared_scalar_result_operand` | `src/backend/mir/aarch64/codegen/alu.cpp`, `alu.hpp` | prepared names, value locations, storage plan, BIR values | `OperandRecord`, `RegisterOperand`, stack publication offset | necessary prepared adapter | Prepared scalar ALU record tests and scalar record contract tests if reshaped. |
| `make_prepared_scalar_alu_record` | `src/backend/mir/aarch64/codegen/alu.cpp`, `alu.hpp` | prepared names, value locations, storage plan, BIR binary | `PreparedScalarAluRecordResult` / `ScalarAluRecord` | necessary prepared adapter | `backend_aarch64_prepared_scalar_alu_records` and scalar ALU record tests; retain opcode/immediate/reduction semantics. |
| `make_prepared_scalar_alu_instruction_record` | `src/backend/mir/aarch64/codegen/alu.cpp`, `alu.hpp` | same inputs as `make_prepared_scalar_alu_record` | delegates to `make_scalar_alu_instruction_record(*record)` | redundant wrapper | First bounded candidate: inline at `lower_scalar_instruction` or fold to existing local utility, then run ALU proof below. |
| `make_prepared_scalar_unary_record` | `src/backend/mir/aarch64/codegen/alu.cpp`, `alu.hpp` | prepared names, value locations, storage plan, unary operation/result/operand | `PreparedScalarUnaryRecordResult` / `ScalarUnaryRecord` | necessary prepared adapter | Prepared scalar ALU/scalar record tests; retain register-only result constraints. |
| `make_prepared_scalar_unary_instruction_record` | `src/backend/mir/aarch64/codegen/alu.cpp`, `alu.hpp` | same inputs as `make_prepared_scalar_unary_record` | delegates to `make_scalar_unary_instruction_record(*record)` | redundant wrapper | Same ALU proof as first candidate; can be included if supervisor wants a two-wrapper ALU packet. |
| `make_control_publication_operand`, `control_prepared_scalar_result_operand`, `find_prepared_control_same_block_scalar_producer` | `src/backend/mir/aarch64/codegen/alu.cpp`, `alu.hpp` | prepared value homes/storage, same-block scalar producers, emitted scalar state | operand/result facts for compare/select publication | necessary prepared adapter | Branch/control and prepared branch tests; do not fold into scalar ALU record helpers without preserving load-source fallback behavior. |
| `lower_scalar_compare_publication` / `lower_scalar_select_publication` | `src/backend/mir/aarch64/codegen/alu.cpp`, `alu.hpp` | prepared value homes/storage, source producers/select-chain materialization | assembler `MachineInstruction`; records emitted scalar register | target-local emission | `backend_aarch64_prepared_branch_records`, branch control lowering, scalar compare/select route tests. |
| `lower_scalar_control_value_instruction` | `src/backend/mir/aarch64/codegen/alu.cpp`, `alu.hpp` | BIR control instruction, prepared facts consumed by compare/select lowerers | dispatch to compare/select/scalar lowering | existing-local-utility fold | Control-flow tests if dispatch is corrected; note current final binary branch is unreachable after the first binary branch and should be handled only in an ALU/control packet. |

### First Bounded Implementation Candidate

Contract `make_prepared_scalar_alu_instruction_record` in
`src/backend/mir/aarch64/codegen/alu.cpp` and its declaration in
`src/backend/mir/aarch64/codegen/alu.hpp` by folding the call site in
`lower_scalar_instruction` to consume `make_prepared_scalar_alu_record`
directly and then call existing local utility `make_scalar_alu_instruction_record`.
This removes a pure forwarding wrapper while keeping prepared fact lookup and
instruction-record construction in the AArch64 ALU owner.

Suggested proof for that candidate:

```sh
cmake --build build --target c4c && ctest --test-dir build -R "backend_aarch64_prepared_scalar_alu_records|backend_aarch64_scalar_alu_records|backend_aarch64_scalar_record_contract|backend_codegen_route_aarch64_alu_unpublished_load_local_after_call|backend_codegen_route_aarch64_alu_unpublished_load_local_call_boundary|backend_codegen_route_aarch64_scalar_fp_literal_add_publishes_fpr_result" --output-on-failure
```

## Suggested Next

Delegate the first bounded ALU contraction candidate above. Owned files should
be `src/backend/mir/aarch64/codegen/alu.cpp`,
`src/backend/mir/aarch64/codegen/alu.hpp`, and `todo.md`.

## Watchouts

- The audit found several target-local emission helpers that should not be
  treated as cleanup, especially indirect callee materialization, call-boundary
  source materialization, store-source publication lowering, and scalar
  compare/select publication.
- `make_prepared_frame_slot_load_memory_instruction_record` is also a redundant
  wrapper, but memory overload traffic makes the ALU instruction-record wrapper
  the smaller first candidate.
- `lower_scalar_control_value_instruction` has an apparent unreachable scalar
  ALU fallback branch after the first binary branch. That is a separate
  ALU/control-flow packet, not part of the first wrapper contraction.
- Do not modify unrelated transient `review/` artifacts or rejected test logs.
- Treat target-local emission and necessary prepared adapters as retained
  wrappers, not cleanup candidates.

## Proof

Inventory-only packet; no build required by the supervisor. Ran:

```sh
git diff --check
```

Proof log: not updated because this packet has no build/test proof contract.
