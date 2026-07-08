Status: Active
Source Idea Path: ideas/open/612_rv64_instruction_fragment_consumers.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Broaden Only Within Pointer BinaryInst Authority

# Current Packet

## Just Finished

Completed Step 3, "Broaden Only Within Pointer BinaryInst Authority", by
refreshing the direct-probe classification after commit `fa08c77ff`. No code or
expectation files were changed.

Fresh direct-probe classification:
- `src/strct-pack-3.c` confirms movement from the former pointer `BinaryInst`
  stop at `function=f`, `block=entry`, `instruction_index=8`,
  `instruction_kind=BinaryInst`, `owner=ptr %t9`. The current first stop is
  `prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination`
  at `function=f`, `block=entry`, `instruction_index=14`, with
  `diagnostic_owner=rv64_prepared_move_bundle_consumer` and
  `fragment_status=producer_authority_missing_for_register_fan_in_stack_destination`.
  The later same-shape `%t26` pointer-add row remains unobserved because this
  move-bundle blocker occurs first.
- `src/930526-1.c` remains intentionally excluded at
  `unsupported_instruction_fragment`, `function=f`, `block=block_1`,
  `instruction_index=4`, `instruction_kind=BinaryInst`, `owner=ptr %t10`.
  Its base `%lv.m.0` is still a stack-slot/frame-slot local address base, so
  the blocker remains address-materialization/prepared-fact ownership rather
  than in-scope pointer `BinaryInst` authority.

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

Conclusion: no remaining directly observable blocker in this refreshed packet
belongs to in-scope pointer `BinaryInst` authority. The visible `strct-pack-3.c`
blocker is a move-bundle owner, `930526-1.c` remains an excluded
address-materialization/prepared-fact owner, and the remaining guard rows stay
under their separate owners. The pointer runbook is ready for Step 4
close-readiness/split classification rather than move-bundle implementation.

## Suggested Next

Next coherent packet should perform Step 4 residual split or close-readiness
classification for idea 612. Do not start move-bundle implementation from this
pointer `BinaryInst` runbook unless the supervisor opens or activates that
separate owner route.

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
  owned pointer `BinaryInst` row to a separate move-bundle authority blocker.
- Direct-probe logs for this refresh are under
  `build/agent_state/step3_pointer_binaryinst_refresh/`.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

Result: passed, 346/346 backend tests. Proof log: `test_after.log`.
