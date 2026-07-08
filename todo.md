Status: Active
Source Idea Path: ideas/open/612_rv64_instruction_fragment_consumers.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Broaden Only Within Pointer BinaryInst Authority

# Current Packet

## Just Finished

Completed Step 3, "Broaden Only Within Pointer BinaryInst Authority", by
refreshing direct object-route probes after commit `029f92780` and classifying
the remaining sampled pointer `BinaryInst` residuals without touching code,
tests, expectations, markers, `plan.md`, or idea files.

Fresh direct-probe movement after the pointer-add consumer is still visible:
- `src/20000801-1.c` is past the sampled `foo` entry pointer `BinaryInst`
  (`owner=ptr %t2`) and now stops at
  `unsupported_branch_stack_load_authority`, `foo`, `block_1`,
  `terminator_instruction_index=3`, branch lhs `%t3`.
- `src/loop-2f.c` is past the sampled `f`/`logic.end.7` pointer
  `BinaryInst` (`owner=ptr %t10`) and now stops at `main` entry
  `CallInst`, `instruction_index=0`, `unsupported_instruction_fragment`.
- `src/pr41395-2.c` is past the sampled `foo` entry pointer `BinaryInst`
  (`owner=ptr %t3`) and now stops at
  `ambiguous_non_parallel_multi_source_stack_destination`, `foo` entry
  `instruction_index=9`, `fragment_status=producer_authority_missing_for_register_fan_in_stack_destination`.

Fresh residual pointer `BinaryInst` rows:
- `src/930526-1.c`: still `unsupported_instruction_fragment`, `function=f`,
  `block=block_1`, `block_index=3`, `instruction_index=4`,
  `instruction_kind=BinaryInst`, `owner=ptr %t10`.
  Prepared BIR shows `%t10 = bir.add ptr %lv.m.0, %t10.byte_offset`;
  `%lv.m.0` is an address-exposed local-slot/frame-slot base
  (`object #25`, `type=i32`, `address_exposed=yes`,
  `permanent_home_slot=yes`) and also appears as a prepared frame-slot home
  (`home %lv.m.0 value_id=10 kind=stack_slot slot_id=203 offset=912`).
  `%t10.byte_offset` is stack-slot and `%t10` is register `t0`.
  Classification: this should not be folded into the generic pointer-add
  consumer by treating a frame-slot home as a pointer register. It needs a
  stricter address-materialization handoff or strengthened prepared facts for
  frame-slot address bases before RV64 consumes the pointer add.
- `src/strct-pack-3.c`: still `unsupported_instruction_fragment`,
  `function=f`, `block=entry`, `block_index=0`, `instruction_index=8`,
  `instruction_kind=BinaryInst`, `owner=ptr %t9`.
  Prepared BIR shows `%t9 = bir.add ptr %p.ap, %t9.byte_offset.static`;
  `%p.ap` is register `a0`, `%t9.byte_offset.static` is stack-slot
  `slot_id=14 offset=32`, and `%t9` is register `s2`. The same function has
  the adjacent same-shape row `%t26 = bir.add ptr %p.ap,
  %t26.byte_offset.static` in `block_1`; `%t26.byte_offset.static` is
  stack-slot `slot_id=24 offset=72`, and `%t26` is also register `s2`.
  Classification: this has a bounded adjacent implementation home only if the
  consumer proves target identity and scratch safety when the dynamic offset
  producer and pointer-add result share the same target register. It should be
  expressed as a general prepared-home/target-identity extension for register
  base plus stack static-offset plus register result, not as a
  `strct-pack-3.c` special case.

Negative guards from the same refreshed probe set stayed under their expected
owners:
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

Next bounded implementation packet, if the supervisor wants to keep Step 3 in
execution, should target only the `src/strct-pack-3.c` class: register pointer
base, stack-slot `.byte_offset.static`, register pointer result, with explicit
target identity and scratch-safety proof when the intermediate dynamic offset
and result use the same target register. Positive rows for that packet are
`src/strct-pack-3.c` `f` entry `instruction_index=8`, `owner=ptr %t9`, and the
same-shape later row `f` `block_1` `owner=ptr %t26` once the first blocker
moves. Required guards are the refreshed guard list above, especially
`src/20021120-1.c`, `src/990524-1.c`, `src/20000722-1.c`,
`src/ptr-arith-1.c`, and scalar `src/931110-1.c`.

Do not combine `src/930526-1.c` into that packet. Its base is an
address-exposed local frame-slot value, so it should go to a stricter
address-materialization/prepared-fact packet or to Step 4 classification if
that owner is outside the current pointer `BinaryInst` authority.

## Watchouts

- A `src/strct-pack-3.c` packet must not accept register-name strings without
  prepared target identity; it needs the same explicit register authority as
  the first pointer-add consumer.
- Do not treat `%lv.m.0` in `src/930526-1.c` as an ordinary pointer home. The
  current prepared facts describe a frame-slot/local-slot address base and need
  a stricter address-materialization handoff.
- Do not relax immediate-only or loaded-base plus scaled-offset policy; the
  pointer-arithmetic guards must stay under `unsupported_pointer_arithmetic`.
- Do not broaden into local-memory, branch, select, cast, ABI, global, inline
  asm, terminator, scalar-binary, or move-bundle owners. Those remained stable
  in the refreshed probe set.
- The three moved positive rows still expose downstream blockers, not full
  object success.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

Result: passed. Proof log: `test_after.log`.
