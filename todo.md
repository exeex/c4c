Status: Active
Source Idea Path: ideas/open/386_rv64_object_route_unsupported_instruction_fragment.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture Unsupported Instruction Fragment Evidence

# Current Packet

## Just Finished

Step 1 audit captured the prepared/BIR instruction fragments behind the
generic `unsupported_instruction_fragment` diagnostic for both representatives.

`tests/c/external/gcc_torture/src/va-arg-13.c`:

- Exact fragment: `bir.call void dummy(ptr %t7)` in `test`, `entry`,
  block index 0, instruction index 9. A second same-shape fragment appears
  later as `bir.call void dummy(ptr %t14)` at instruction index 16, but the
  traversal reaches inst 9 first.
- Instruction kind: `bir::CallInst`, same-module direct call, void return,
  one pointer argument.
- Operand/types: arg0 is `ptr %t7`; call arg source records
  `index=0 encoding=frame_slot source_value=%t7 selection=frame_slot_address
  selection_value=%t7`.
- Prepared value/storage facts: `%t7` has value id 15, prepared storage
  `encoding=frame_slot bank=gpr spill_slot=slot#9+stack48 width=1 slot_id=#9
  stack_offset=48`; value home reports `kind=stack_slot slot_id=9 offset=48`.
  The address materialization fact for this call is
  `block=entry inst_index=9 kind=frame_slot result=%t7 offset=24`; the
  selection metadata points at the address-exposed local slot
  `selection_source_slot=#7 selection_source_stack_offset=24
  selection_source_size=8 selection_source_align=8`.
- Prepared object-route call facts: callsite `block=0 inst=9 wrapper=same_module
  callee=dummy args=1`; arg0 is `value_bank=gpr`, `source_encoding=frame_slot`,
  `source_value_id=15`, `source_slot=#9`, `source_stack_offset=48`,
  destination `gpr:call_argument#0/w1 dest_reg=a0`, with
  `arg.source_selection=frame_slot_address`,
  `selection_materialization_block=entry`,
  `selection_materialization_inst=9`,
  `selection_materialization_slot=#7`,
  `selection_materialization_offset=24`,
  `missing_frame_slot_arg_publication=yes`,
  `missing_frame_slot_arg_kind=frame_slot_address`,
  `missing_frame_slot_arg_source_materializes_address=yes`,
  `missing_frame_slot_arg_may_emit_local_payload=no`.
- Current diagnostic path: `prepared_function_to_object_function` walks the
  prepared object traversal, handles an `Instruction` event, calls
  `fragment_for_prepared_instruction`, then `fragment_for_prepared_call`.
  The call is not a variadic helper. `fragment_for_prepared_call` accepts the
  wrapper but rejects the GPR argument because the argument source selection is
  `PreparedCallArgumentSourceSelectionKind::FrameSlotAddress`; the current GPR
  argument switch only supports ordinary frame-slot values, prior
  preservation, local frame address materialization, immediates, registers, and
  symbol addresses. No narrower helper or memory diagnostic applies, so the
  fallback emits
  `unsupported_instruction_fragment: BIR instruction requires unsupported RV64
  object lowering`.

`tests/c/external/gcc_torture/src/920908-1.c`:

- Exact fragment: `bir.call void f(ptr sret(size=4, align=4) %t8, i32 2,
  i64 %t4, i64 %t7)` in `main`, `entry`, block index 0, instruction index 8.
- Instruction kind: `bir::CallInst`, same-module direct call, void return with
  ABI memory return/sret argument.
- Operand/types: arg0 is `ptr sret(size=4, align=4) %t8`, arg1 is `i32 2`,
  arg2 is `i64 %t4`, arg3 is `i64 %t7`; call arg sources record arg0 as
  `encoding=frame_slot source_value=%t8 selection=frame_slot_address`, arg1
  immediate, arg2 register `%t4`, arg3 register `%t7`.
- Prepared value/storage facts: `%t8` has value id 17, prepared storage
  `encoding=register bank=gpr placement=gpr:callee_saved#0/w1 reg=s1 width=1`;
  value home reports `kind=register reg=s1`. The actual memory-return home is
  frame slot #7 at stack offset 16, size 4, align 4, from object `%t8.0`.
  The address materialization fact is `block=entry inst_index=8 kind=frame_slot
  result=%t8 offset=16`.
- Prepared object-route call facts: callsite `block=0 inst=8 wrapper=same_module
  callee=f args=4 memory_return=%t8 memory_home=frame_slot sret_arg=0
  memory_stack_offset=16`; detailed call plan records
  `memory_encoding=frame_slot sret_arg_index=0 memory_slot=#7
  memory_stack_offset=16 memory_size=4 memory_align=4`. Arg0 is
  `value_bank=aggregate_address`, `source_encoding=register`,
  `source_value_id=17`, `source_reg=s1`, `dest_bank=none`, with
  `arg.source_selection=local_frame_address_materialization`,
  `selection_source_slot=#7`, `selection_source_stack_offset=16`,
  `selection_materialization_block=entry`,
  `selection_materialization_inst=8`,
  `selection_materialization_slot=#7`,
  `selection_materialization_offset=16`. Arg1 maps immediate 2 to `a1`, arg2
  maps `%t4`/`t0` to `a2`, and arg3 maps `%t7`/`s2` to `a3`.
- Current diagnostic path: `prepared_function_to_object_function` reaches the
  `main` instruction event at block 0 inst 8, calls
  `fragment_for_prepared_instruction`, then `fragment_for_prepared_call`.
  The call is not a variadic helper. `fragment_for_prepared_call` returns empty
  at its admission gate because `call_plan->memory_return.has_value()` is true.
  No variadic-helper diagnostic applies and `diagnose_unsupported_prepared_instruction_fragment`
  has no call-specific narrower diagnostic, so the fallback emits
  `unsupported_instruction_fragment: BIR instruction requires unsupported RV64
  object lowering`.

Comparison: both representatives currently fail on `bir::CallInst` object-route
lowering, but they are separate semantic call families. `va-arg-13.c` is a
scalar GPR pointer argument whose source selection is `FrameSlotAddress` for an
address-exposed local frame slot. `920908-1.c` is an ABI memory-return/sret
same-module call rejected before argument lowering by the `memory_return`
admission gate, even though its sret address is represented as local frame
address materialization.

## Suggested Next

Delegate Step 2 to classify these as two call-lowering families: frame-slot
address GPR call arguments versus same-module memory-return/sret calls. Keep
them split unless the classifier finds an existing common call-address
materialization abstraction that already owns both.

## Watchouts

- Do not infer the missing lowering from testcase names.
- Do not reopen idea 374 parameter-home support unless fresh evidence proves
  the diagnostic still comes from parameter-home facts.
- Do not downgrade expectations or add named-case-only handling.
- `920908-1.c` also contains supported `llvm.va_arg.aggregate` helper facts in
  `f`; the current generic diagnostic is not the variadic helper path.
- `va-arg-13.c` has two same-shape `dummy` calls, but the first failing
  fragment is block 0 inst 9; inst 16 is follow-on evidence, not the first
  diagnostic trigger.

## Proof

Audit-only. Ran read-only diagnostics/dumps without writing `test_before.log`
or `test_after.log`:

- `build/c4cll -I tests/c/external/gcc_torture --target riscv64-linux-gnu
  --codegen obj tests/c/external/gcc_torture/src/va-arg-13.c -o
  /tmp/va-arg-13.o` reproduced the generic unsupported instruction diagnostic.
- `build/c4cll -I tests/c/external/gcc_torture --target riscv64-linux-gnu
  --codegen obj tests/c/external/gcc_torture/src/920908-1.c -o
  /tmp/920908-1.o` reproduced the same generic diagnostic.
- `build/c4cll -I tests/c/external/gcc_torture --target riscv64-linux-gnu
  --dump-prepared-bir --mir-focus-function test
  tests/c/external/gcc_torture/src/va-arg-13.c`
- `build/c4cll -I tests/c/external/gcc_torture --target riscv64-linux-gnu
  --dump-prepared-bir --mir-focus-function main
  tests/c/external/gcc_torture/src/920908-1.c`
