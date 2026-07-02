Status: Active
Source Idea Path: ideas/open/515_rv64_bankless_conversion_adjacent_stack_slot_moves.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Rebuild Bankless Conversion Evidence

# Current Packet

## Just Finished

Step 1 - Rebuild Bankless Conversion Evidence is complete for
`tests/c/external/gcc_torture/src/pr69447.c`.

Representative focused probes:

- `./build/c4cll --dump-bir --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/pr69447.c`
- `./build/c4cll --dump-prepared-bir --target riscv64-unknown-linux-gnu --mir-focus-function foo tests/c/external/gcc_torture/src/pr69447.c`
- `./build/c4cll --dump-prepared-bir --target riscv64-unknown-linux-gnu --mir-focus-function foo --mir-focus-value t8 tests/c/external/gcc_torture/src/pr69447.c`
- `./build/c4cll --dump-prepared-bir --target riscv64-unknown-linux-gnu --mir-focus-function foo --mir-focus-value t9 tests/c/external/gcc_torture/src/pr69447.c`
- `./build/c4cll --codegen obj --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/pr69447.c -o /tmp/pr69447.o`

Semantic source shape in `foo`:

- `%t8 = bir.load_local i16 %lv.param.u16_1`
- `%t9 = bir.zext i16 %t8 to i64`
- `%t10 = bir.or i64 %t9, %t7`

Prepared move bundle:

- `move_bundle phase=before_instruction authority=none block_index=0 instruction_index=14`
- Single move, so cardinality is 1 and `parallel_copy=no` in the object-route
  diagnostic.
- Move record: `from_value_id=15` to `to_value_id=16`,
  `destination_kind=value`, `destination_storage=stack_slot`, `op_kind=move`,
  `reason=consumer_stack_to_stack`, `uses_cycle_temp_source=no`.

Homes and stack layout:

- Source `%t8`, `value_id=15`, has `home kind=stack_slot slot_id=18
  offset=104`; stack object `#18` is `type=i16 size=2 align=2`, slot `#18`
  offset `104`, size `2`, align `2`.
- Destination `%t9`, `value_id=16`, has `home kind=stack_slot slot_id=19
  offset=112`; stack object `#19` is `type=i64 size=8 align=8`, slot `#19`
  offset `112`, size `8`, align `8`.

Storage-plan entries:

- Source `%t8 value_id=15 encoding=frame_slot bank=none
  spill_slot=slot#18+stack104 width=1 slot_id=#18 stack_offset=104`.
- Destination `%t9 value_id=16 encoding=frame_slot bank=gpr
  spill_slot=slot#19+stack112 width=1 slot_id=#19 stack_offset=112`.

Current rejection site:

- `./build/c4cll --codegen obj --target riscv64-unknown-linux-gnu ...`
  rejects in the RV64 object route with
  `unsupported_move_bundle_target_shape: prepared move bundle requires
  unsupported RV64 moves`.
- Exact diagnostic facts include `event_kind=before_instruction_copies`,
  `function=foo`, `block_index=0`, `block_label=entry`,
  `instruction_index=14`, `phase=before_instruction`,
  `authority=none`, `move_count=1`, `parallel_copy=no`,
  `move[0].from_value_id=15`, `move[0].to_value_id=16`,
  `move[0].source_home_kind=stack_slot`, `move[0].source_type=i16`,
  `move[0].destination_home_kind=stack_slot`,
  `move[0].destination_type=i64`, and
  `fragment_status=generic_move_bundle_materialization_failed`.

Classification:

- Not idea 513's coherent same-scalar stack-to-stack shape: both homes are
  stack slots, but source scalar type is `i16`, destination scalar type is
  `i64`, and the source storage plan is `bank=none`.
- Not idea 514's register-source shape: the failing move source home is a
  stack slot, not a register.
- Not idea 516's multi-source shape: the failing bundle has exactly one move.
- The first owner to investigate is conversion-adjacent movement authority.
  The source slot is bankless, but assigning a GPR bank alone would still leave
  a raw `op_kind=move` from `i16` storage into an `i64` destination for a
  semantic `zext`; that needs an explicit conversion contract or an
  owner-specific rejection before generic stack-copy materialization.

## Suggested Next

Proceed to Step 2 by tracing the prepared move-bundle producer and RV64 object
emission rejection path for the `instruction_index=14` `%t8` to `%t9` `zext`
case. Define the fail-closed boundary for missing conversion authority first,
then separately decide whether `bank=none` on `%t8` is a storage
classification omission or an intentional unsupported frame-slot fact.

## Watchouts

The current rejection is precise enough to expose the bad facts, but it still
lands as `generic_move_bundle_materialization_failed`; Step 2 should determine
whether the owner-specific diagnostic belongs in prepared movement
classification or RV64 object emission. Do not treat `width=1` on both storage
plans as same-width scalar authority: the stack objects are `i16` size 2 and
`i64` size 8.

## Proof

Evidence-only packet. No code-changing proof was required, and
`test_after.log` was intentionally untouched per the delegation.
