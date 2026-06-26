Status: Active
Source Idea Path: ideas/open/392_rv64_va_list_expression_call_argument_value_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture The Va List Value Boundary

# Current Packet

## Just Finished

Step 1 captured the RV64 `va_list` expression call-argument value boundary for
`tests/c/external/gcc_torture/src/va-arg-13.c`. The initialized `va_list`
payload source is `%lv.state.8`: prepared value id 10, register `s1`, with
`va_start` helper operands for block 0 inst 3 and inst 12 mapping
`dst_va_list=%lv.state.8:stack_slot:slot=#13:offset=128` and
`dst_va_list_addr=%lv.state.8:register:reg=s1`. The variadic save-area route is
present: `rv64.incoming_variadic_gpr_publication` stores incoming `a1` through
`a7` into overflow/save slot #12 starting at stack offset 72, so idea 391 is not
the current owner.

The first `dummy` call copies the initialized payload through
`%t7.memcpy.copy.0`: semantic/prepared BIR loads `%lv.state.8` at inst 7,
stores that pointer payload into destination object `%t7` at inst 8, and calls
`dummy(ptr %t7)` at inst 9. Prepared storage records
`%t7.memcpy.copy.0` as value id 14 in `t0`, while `%t7` is value id 15,
frame-slot spill slot #9 at stack offset 48, with address materialization
slot #6 offset 24. The second call mirrors this through
`%t14.memcpy.copy.0` value id 18 into `%t14` value id 19, frame-slot spill
slot #10 at stack offset 56, with address materialization slot #7 offset 32,
then calls `dummy` at inst 16.

The exact gap is that prepared call facts still model both call arguments as
frame-slot address selections, not as value-publication copies of the initialized
payload. Callsite inst 9 has arg0 `source_encoding=frame_slot`,
`source_value_id=15`, `source_slot=#9`, `source_stack_offset=48`,
`dest_reg=a0`, `arg.source_selection=frame_slot_address`,
`missing_frame_slot_arg_publication=yes`, and
`missing_frame_slot_arg_may_emit_local_payload=no`. Callsite inst 16 repeats the
same shape for `source_value_id=19`, `source_slot=#10`,
`source_stack_offset=56`. Store-source publication facts exist for inst 8
`source=%t7.memcpy.copy.0` and inst 15 `source=%t14.memcpy.copy.0`, but they do
not yet give the RV64 object call fragment authority to publish that initialized
`va_list` pointer payload into the call-argument object before materializing its
address.

Disassembly from the preserved representative confirms the object symptom after
idea 391: c4c stores `a1` through `a7` into the save area beginning at
`sp+0x48` and `va_start` writes `sp+0x48` into the local `va_list` object at
`sp+0x80`, but before `dummy` it stores `s1`/`sp+0x80` into the argument object
at `sp+0x18` and passes `a0=sp+0x18`. `dummy` loads through that argument
object, so it receives the address of the `va_list` storage instead of the
initialized save-area pointer payload.

## Suggested Next

Execute Step 2: classify a guarded RV64 prepared/object value-publication route
for frame-slot call arguments whose destination object was populated from an
initialized `va_list` payload. The route should require explicit facts linking
the `StoreLocalPublication` source payload (`%t7.memcpy.copy.0` or
`%t14.memcpy.copy.0`) to the frame-slot argument object (`%t7` or `%t14`) before
address materialization, and fail closed when that link is absent, duplicate, or
ambiguous.

## Watchouts

- Do not solve this by hard-coding `va-arg-13.c`, `test`, `dummy`, offset
  `0x80`, offset `0x48`, or the abort branch.
- Do not reopen idea 391 unless evidence shows incoming variadic GPR payloads
  are no longer stored into the backing save area.
- Do not treat `missing_frame_slot_arg_publication=yes` alone as enough
  authority; idea 390 already covered address-argument publication, while this
  packet needs the initialized `va_list` value payload copied into the argument
  object.
- The likely owner is RV64 prepared/object call-argument value publication, not
  variadic prologue setup, `va_start` address materialization, or `va_end`.

## Proof

Evidence-only Step 1. Regenerated semantic/prepared BIR evidence and a focused
fact-gap note under `build/agent_state/392_step1_*`; did not rerun the backend
subset because no implementation or test files changed. The prior backend proof
from Step 2/3/4 remains current, and the root `test_after.log` representative
failure payload was preserved.

Evidence paths:

- `build/agent_state/392_step1_va-arg-13.semantic-bir.log`
- `build/agent_state/392_step1_va-arg-13.prepared-bir.log`
- `build/agent_state/392_step1_va-arg-13.fact-gap.log`
- `build/agent_state/391_step5_va-arg-13.c4c-disasm.log`
- `build/agent_state/391_step5_va-arg-13.clang-disasm.log`
- `build/agent_state/391_step5_va-arg-13.case.log`
- `build/agent_state/391_step5_va-arg-13.qemu-strace.log`
