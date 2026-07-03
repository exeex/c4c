# Current Packet

Status: Active
Source Idea Path: ideas/open/567_rv64_integer_div_rem_instruction_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Pin First Downstream Unsupported Fragment

## Just Finished

Step 2 (`Pin First Downstream Unsupported Fragment`) completed the
evidence/diagnostic packet for representative `src/20001026-1.c`.

Pinned artifact:

- `build/agent_state/567_step2_pinned_fragment.md`

Supporting artifacts:

- `build/agent_state/567_step2_single.allowlist`
- `build/agent_state/567_step2_20001026_trace.log`
- Existing BIR dump: `build/agent_state/567_step1_20001026-1.bir.txt`
- Existing prepared-BIR dump:
  `build/agent_state/567_step1_20001026-1.prepared_bir.txt`

Pinned first fallthrough:

- Case: `src/20001026-1.c`
- Function: `real_value_from_int_cst`
- Block: `block_1`
- Traversal block index: `3`
- Instruction index: `4`
- Instruction kind: `BinaryInst`
- Opcode/result: `add ptr`
- Instruction text: `%t12 = bir.add ptr %lv.r.0, %t12.byte_offset`

Operand/result facts:

- `%lv.r.0`: value id `14`, GPR register home `s1`, placement
  `gpr:callee_saved#0/w1`.
- `%t12.byte_offset`: value id `12`, GPR frame-slot home `slot#22+stack80`.
- `%t12`: value id `13`, pointer frame-slot home `slot#23+stack88`.
- The before-instruction move bundle for block index `3`, instruction index
  `4` moves value ids `14` and `12` into destination value id `13`, but the
  instruction itself still falls through object emission.
- Prepared address-materialization evidence exists for the same source block
  and instruction index:
  `address_materialization block=block_1 inst_index=4 kind=frame_slot result=%lv.r.0 policy=unspecified offset=8 address_space=default`.

Rejecting hook:

- `src/backend/mir/riscv/codegen/object_emission.cpp::prepared_function_to_object_function(...)`
  visits the prepared object traversal instruction event.
- `fragment_for_prepared_instruction(...)` returns `std::nullopt` after the
  frame-address, symbol-address, and generic binary hooks do not claim the
  instruction.
- `fragment_for_prepared_binary(...)` is not the semantic owner because it
  admits only `i32`/`i64` result binaries; the pinned instruction result is
  `ptr`.
- The generic rejection remains:
  `unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering`.

Owner decision:

- This is not a missing raw RV64 div/rem opcode fragment.
- Existing div/rem lowering already had a chance to handle the earlier
  `%t1 = bir.udiv i64 24, 8`.
- The first remaining unsupported fragment is pointer-result local
  address-materialization for a dynamic local aggregate element address:
  `%t12 = bir.add ptr %lv.r.0, %t12.byte_offset`.
- Semantic owner: downstream RV64 prepared pointer/local-address
  materialization, not the integer div/rem opcode route.

## Suggested Next

Plan-owner/supervisor should reroute Step 3 away from div/rem opcode lowering.
The next coherent packet is a narrow lifecycle repair or implementation plan
for pointer-result frame-slot/local-address materialization, using the pinned
`%t12 = bir.add ptr %lv.r.0, %t12.byte_offset` evidence as the first owner.

## Watchouts

- Do not add duplicate `sdiv`/`udiv`/`srem`/`urem` lowering or duplicate the
  existing focused div/rem opcode tests for this packet.
- Do not classify from the generic diagnostic alone; use
  `build/agent_state/567_step2_pinned_fragment.md` as the pinned fragment
  authority.
- The temporary trace instrumentation was removed before return; no
  implementation files are intentionally changed.
- Leave `review/557_step13_vector_local_memory_review.md` untouched.

## Proof

- Evidence-only packet; no CTest proof required.
- Focused representative command:
  `ALLOWLIST=build/agent_state/567_step2_single.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/567_step2_20001026_trace.log 2>&1`
- Focused representative result: exit `1`, expected for the current failing
  row, with the temporary trace pinning the first fallthrough.
- Local validation: `git diff --check -- todo.md`
