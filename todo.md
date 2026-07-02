Status: Active
Source Idea Path: ideas/open/514_rv64_register_source_stack_destination_move_bundles.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Rebuild Post-516 Register-Source Evidence

# Current Packet

## Just Finished

Step 1 - Rebuild Post-516 Register-Source Evidence completed for idea 514.

`tests/c/external/gcc_torture/src/pr27073.c` on
`--target riscv64-unknown-linux-gnu` still reaches RV64 object emission as the
single-move representative, not as idea 516's multi-source case. Focused
prepared dump:

- Command:
  `./build/c4cll --dump-prepared-bir --target riscv64-unknown-linux-gnu --mir-focus-function foo tests/c/external/gcc_torture/src/pr27073.c`
- Prepared bundle: function `foo`, `phase=before_instruction`,
  `authority=none`, `block_index=0`, `instruction_index=1`,
  `move_count=1`, `parallel_copy=no` by object-route diagnostic.
- BIR site: `%t0 = bir.sext i16 %p.count to i32`.
- Move: `from_value_id=4` (`%p.count`) to `to_value_id=10` (`%t0`),
  `destination_kind=value`, `destination_storage=stack_slot`,
  `op_kind=move`, `uses_cycle_temp_source=no`,
  `reason=consumer_stack_to_stack`.
- Source home/storage: `%p.count value_id=4` has `home kind=register reg=a4`;
  storage plan is `encoding=register bank=gpr reg=a4 width=1 units=a4`.
- Destination home/storage: `%t0 value_id=10` has `home kind=stack_slot
  slot_id=25 offset=68`; storage plan is `encoding=frame_slot bank=gpr
  spill_slot=slot#25+stack68 width=1 slot_id=#25 stack_offset=68`.
- Scalar facts: object-route diagnostic reports `source_type=i16` and
  `destination_type=i32`; prepared stack layout records `%p.count` object
  type `i16 size=2 align=2` and `%t0` object type `i32 size=4 align=4`.
- Cardinality/bank/width: one move, GPR bank on both source and destination,
  one RV64 register unit/source storage unit; destination slot is 4 bytes.
- Current rejection site:
  `./build/c4cll --codegen obj --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/pr27073.c -o /tmp/c4c_514_pr27073.o`
  exits 2 with `RISC-V backend object route unsupported prepared module shape:
  unsupported_move_bundle_target_shape ... event_kind=before_instruction_copies
  function=foo block_index=0 block_label=entry instruction_index=1 ...
  authority=none move_count=1 ... move[0].source_home_kind=register
  move[0].source_type=i16 move[0].destination_home_kind=stack_slot
  move[0].destination_type=i32
  fragment_status=generic_move_bundle_materialization_failed`.
- Step 1 authority conclusion: RV64 has explicit prepared bundle, source home,
  destination home, storage plan, bank, width-unit, and cardinality facts to
  decide the single register-source stack-destination owner without guessing
  from row names or ABI order. It does not have authority for a plain same-scalar
  stack-to-stack materializer: the bundle is `authority=none`, the source home
  is explicitly a register, and the scalar types differ across a sign-extension
  BIR instruction (`i16` source, `i32` destination). Step 2 should decide
  whether RV64 object emission may materialize this as a register-source
  extension/store, or whether producer classification must publish/reject a
  more precise conversion-aware contract.

`tests/c/external/gcc_torture/src/20010518-1.c` is now confirmed as the closed
idea 516 boundary rather than the remaining idea 514 single-move
representative. Focused prepared dump:

- Command:
  `./build/c4cll --dump-prepared-bir --target riscv64-unknown-linux-gnu --mir-focus-function add tests/c/external/gcc_torture/src/20010518-1.c`
- Representative bundle: function `add`, `phase=before_instruction`,
  `authority=none`, `block_index=0`, `instruction_index=0`, `move_count=2`.
- BIR site: `%t0 = bir.add i32 %p.a, %p.b`.
- Moves: `from_value_id=0` (`%p.a`, register `a0`) to `to_value_id=13`
  (`%t0`, stack slot) and `from_value_id=1` (`%p.b`, register `a1`) to the
  same `to_value_id=13`, both `destination_kind=value`,
  `destination_storage=stack_slot`, `op_kind=move`, `reason=consumer_stack_to_stack`.
- Source homes/storage: `%p.a value_id=0` has register home/storage `a0`,
  `bank=gpr`, `width=1`; `%p.b value_id=1` has register home/storage `a1`,
  `bank=gpr`, `width=1`.
- Destination home/storage: `%t0 value_id=13` has `home kind=stack_slot
  slot_id=8 offset=32`; storage plan is `encoding=frame_slot bank=gpr
  spill_slot=slot#8+stack32 width=1 slot_id=#8 stack_offset=32`.
- Scalar facts: all involved values are `i32`; destination slot is 4 bytes.
- Current rejection site:
  `./build/c4cll --codegen obj --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/20010518-1.c -o /tmp/c4c_514_20010518.o`
  exits 2 with `prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination:
  prepared move-bundle classifier rejected ambiguous non-parallel multi-source
  stack-destination authority`.
- Post-516 confirmation: yes, `20010518-1.c` now rejects at
  `prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination`
  before RV64 object emission tries to materialize the ambiguous
  `authority=none`, `move_count=2`, non-parallel multi-source stack-destination
  bundle. Keep it separate from the remaining `pr27073.c` single-move decision.

## Suggested Next

Execute Step 2 by tracing the RV64 prepared move-bundle consumer predicate and
deciding the minimal accepted or rejected contract for the `pr27073.c`
single-move register-source stack-destination shape with `i16 -> i32`
conversion facts.

## Watchouts

Keep the single register-source stack-destination owner separate from idea
516's closed multi-source producer/classification contract. Do not infer
source or destination homes from row names, source order, ABI formulas,
argument indexes, local names, or raw BIR text.

For `pr27073.c`, do not classify the first failing bundle as idea 513's
coherent same-scalar stack-slot-to-stack-slot case: its source home/storage is
explicitly register `a4`, and the current BIR instruction is a sign extension
from `i16` to `i32`. For `20010518-1.c`, do not reopen materialization unless
the classifier category regresses from
`ambiguous_non_parallel_multi_source_stack_destination`.

## Proof

Evidence-only packet; no implementation or test changes were made, and the
delegation explicitly prohibited touching `test_before.log` or
`test_after.log`.

Commands run:

- `./build/c4cll --dump-prepared-bir --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/pr27073.c`
- `./build/c4cll --dump-prepared-bir --target riscv64-unknown-linux-gnu --mir-focus-function foo tests/c/external/gcc_torture/src/pr27073.c`
- `./build/c4cll --codegen obj --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/pr27073.c -o /tmp/c4c_514_pr27073.o`
- `./build/c4cll --dump-prepared-bir --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/20010518-1.c`
- `./build/c4cll --dump-prepared-bir --target riscv64-unknown-linux-gnu --mir-focus-function add tests/c/external/gcc_torture/src/20010518-1.c`
- `./build/c4cll --codegen obj --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/20010518-1.c -o /tmp/c4c_514_20010518.o`

Scratch evidence paths: `/tmp/c4c_514_pr27073_prepared.txt`,
`/tmp/c4c_514_pr27073_foo_prepared.txt`, `/tmp/c4c_514_pr27073_obj.txt`,
`/tmp/c4c_514_20010518_prepared.txt`,
`/tmp/c4c_514_20010518_add_prepared.txt`, and
`/tmp/c4c_514_20010518_obj.txt`.
