Status: Active
Source Idea Path: ideas/open/390_rv64_va_list_value_publication_copy_runtime_abort.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture The `va_list` Value Publication Boundary

# Current Packet

## Just Finished

Step 1 captured the `va-arg-13.c` RV64 value-publication boundary.

Evidence paths:

- `build/agent_state/390_step1_va_arg_13.test.bir.log`
- `build/agent_state/390_step1_va_arg_13.test.prepared_bir.log`
- `build/agent_state/390_step1_va_arg_13.dummy.prepared_bir.log`
- `build/agent_state/390_step1_va_arg_13.objdump.log`

Captured facts:

- The initialized `va_start` destination is `%lv.state.8`, prepared
  value id 10. Prepared storage places it in `s1`, and the variadic helper
  operands identify `dst_va_list=%lv.state.8:stack_slot:slot=#13:offset=72`
  with `dst_va_list_addr=%lv.state.8:register:reg=s1`.
- The first later consumer is `%t7`: BIR loads `%t7.memcpy.copy.0` from
  `%lv.state.8`, stores that into `addr %t7`, and calls `dummy(ptr %t7)`.
  Prepared storage gives `%t7` value id 15 as frame slot #9 at stack+48, with
  address-materialization/home slot #6 at offset 24.
- The second later consumer is `%t14`: BIR loads `%t14.memcpy.copy.0` from
  `%lv.state.8`, stores that into `addr %t14`, and calls `dummy(ptr %t14)`.
  Prepared storage gives `%t14` value id 19 as frame slot #10 at stack+56, with
  address-materialization/home slot #7 at offset 32.
- Both `dummy` call arguments are prepared as `selection=frame_slot_address`
  and are explicitly marked `missing_frame_slot_arg_publication=yes` /
  `missing_frame_slot_arg_kind=frame_slot_address` with
  `missing_frame_slot_arg_may_emit_local_payload=no`.
- Object evidence matches the prepared split: `va_start` stores the overflow
  cursor to `sp+72` (`addi s1,sp,72`; `addi t1,sp,72`; `sd t1,0(s1)`), but
  the later copies load from ordinary `%lv.state.8` member storage at `sp+8`
  and store those loaded values into the call-argument objects at `sp+24` and
  `sp+32` before passing their addresses to `dummy`.
- The likely owner is the RV64 object-route boundary between
  `fragment_for_prepared_variadic_va_start` and `fragment_for_prepared_call`:
  either publish/copy the initialized prepared `va_start` destination payload
  into the by-value `va_list` copy slots, or fail closed when the prepared
  frame-slot-address argument lacks the publication fact needed to do so.

## Suggested Next

Execute Step 2: classify the narrow route for `%lv.state.8` value publication
to the `%t7` / `%t14` by-value `va_list` copy slots, choosing between
publishing back to ordinary member storage, copying from the prepared
destination slot at the frame-slot-address call boundary, or modeling an
explicit prepared alias/publication fact.

## Watchouts

- Keep idea 389 `va_start` destination-address materialization closed.
- Keep idea 388 `va_end`, idea 386 frame-slot-address GPR call arguments, and idea 387 sret out of scope.
- Do not hard-code `va-arg-13.c`, `test`, `dummy`, registers, stack offsets, or the abort branch.
- Do not broaden this packet into a full `va_arg`, aggregate, or call ABI rewrite.
- The prepared BIR already has store-source facts for the stores into `%t7`
  and `%t14`, but the call-argument plan still marks those frame-slot-address
  arguments as missing publication; do not treat the address materialization
  alone as proof that the initialized `va_list` payload reached `dummy`.

## Proof

Command run:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed.

Proof log: `test_after.log`.
