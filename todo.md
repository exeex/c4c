Status: Active
Source Idea Path: ideas/open/612_rv64_instruction_fragment_consumers.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Broaden Only Within Pointer BinaryInst Authority

# Current Packet

## Just Finished

Completed Step 3, "Broaden Only Within Pointer BinaryInst Authority", by
implementing the bounded adjacent pointer `BinaryInst` class for register
pointer base plus stack-slot `.byte_offset.static` plus register pointer result.
The RV64 object consumer now derives explicit GPR identity from prepared home
identity, formal parameter ABI identity, or storage-plan register placement,
and derives regalloc spill-slot offsets from storage-plan frame-slot facts. The
stack-offset add path loads the stack offset through the result register when
base and result are distinct explicit GPRs, proving the `%t9.byte_offset`/`%t9`
same-target scratch case without filename-shaped logic. The consumer now
requires the pointer base itself to have explicit GPR/formal/storage-plan
register authority; stack-slot/frame-slot/local-slot pointer bases fail closed.
Unit coverage includes a stack-base pointer-add rejection.

Fresh direct-probe movement:
- `src/strct-pack-3.c` moved past `function=f`, `block=entry`,
  `instruction_index=8`, `instruction_kind=BinaryInst`, `owner=ptr %t9`.
  It now stops later at
  `ambiguous_non_parallel_multi_source_stack_destination`,
  `function=f`, `block=entry`, `instruction_index=14`,
  `fragment_status=producer_authority_missing_for_register_fan_in_stack_destination`.
  The later same-shape `%t26` row in `block_1` remains unobserved because this
  new move-bundle blocker occurs first.
- `src/930526-1.c` is intentionally excluded and remains at its prior
  `unsupported_instruction_fragment`, `function=f`, `block=block_1`,
  `instruction_index=4`, `instruction_kind=BinaryInst`, `owner=ptr %t10`.
  Its base `%lv.m.0` is a stack-slot/frame-slot local address base, so this
  packet does not consume it.

Guard checks stayed under their expected owners:
- `src/20021120-1.c` and `src/990524-1.c` remain
  `unsupported_pointer_arithmetic`.
- `src/20000722-1.c` and `src/ptr-arith-1.c` remain
  `unsupported_local_memory_access`.
- `src/930930-1.c` remains branch stack-load freshness.
- `src/20000815-1.c` remains `SelectInst` `unsupported_instruction_fragment`.
- `src/20010604-1.c` remains `CastInst` `unsupported_instruction_fragment`.
- `src/20000603-1.c` remains `unsupported_call_abi`.
- `src/20020213-1.c` remains `unsupported_global_data`.
- `src/20000422-1.c` remains `unsupported_move_bundle_target_shape`.
- `src/pr40022.c` remains `unsupported_inline_asm_fragment`.
- `src/20000801-2.c` remains `unsupported_terminator_fragment`.
- `src/931110-1.c` remains scalar `BinaryInst owner=i16`
  `unsupported_instruction_fragment`.

## Suggested Next

Next coherent packet should classify or repair the new `src/strct-pack-3.c`
entry move-bundle blocker at `instruction_index=14` before expecting the later
same-shape `%t26` pointer-add row to become observable.

## Watchouts

- The new pointer-add path still must not accept register-name-only authority:
  rendered names are only cross-checks after home identity, formal ABI identity,
  or storage-plan placement identity has supplied the physical GPR.
- Do not treat `%lv.m.0` in `src/930526-1.c` as an ordinary pointer home. The
  current prepared facts describe a frame-slot/local-slot address base; this
  packet keeps that class out of the pointer-add consumer.
- Do not relax immediate-only or loaded-base plus scaled-offset policy; the
  pointer-arithmetic guards must stay under `unsupported_pointer_arithmetic`.
- Do not broaden into local-memory, branch, select, cast, ABI, global, inline
  asm, terminator, scalar-binary, or move-bundle owners. Those remained stable
  in the refreshed probe set.
- This is not full object success for `strct-pack-3.c`; it is movement of the
  owned pointer `BinaryInst` row to the next authority blocker.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

Result: passed. Proof log: `test_after.log`.
