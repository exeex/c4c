Status: Active
Source Idea Path: ideas/open/514_rv64_register_source_stack_destination_move_bundles.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define The Minimal Single-Move Contract

# Current Packet

## Just Finished

Step 2 - Define The Minimal Single-Move Contract completed for idea 514.

Accepted semantic boundary: RV64 object emission may materialize only the
existing same-width single register-source to stack-destination prepared move
shape owned by `fragment_for_prepared_move_bundle` in
`src/backend/mir/riscv/codegen/object_emission.cpp`.

The accepted predicate is:

- traversal event is a copy event accepted by
  `classify_prepared_object_move_bundle_consumer`
- `phase=before_instruction`, `authority=none`, `destination_kind=value`,
  `destination_storage=stack_slot`, `op_kind=move`, no cycle-temp source, no
  immediate source, no destination stack-offset override, contiguous width 1,
  and no occupied destination registers
- `reason=consumer_register_to_stack`
- exactly one `consumer_register_to_stack` move in the bundle
- source home exists, is `kind=register`, resolves to an RV64 GPR, and any
  storage-plan endpoint agrees with the same GPR register
- destination home exists, is a stack slot with an encodable absolute stack
  offset and a scalar destination type whose size is supported by the RV64
  store helper
- source scalar size is authoritative and equals destination scalar size
- emitted store is the destination-size store from the explicit source GPR to
  the explicit destination stack slot (`sb`, `sh`, `sw`, or `sd` as permitted
  by `rv64_load_store_funct3_for_size`)

Rejected semantic boundary for `pr27073.c`: the current first failing bundle is
not accepted by the minimal contract. It is a single explicit register-source
stack-destination movement, but it arrives as `reason=consumer_stack_to_stack`
for `%t0 = bir.sext i16 %p.count to i32`, with source home `register a4`,
destination home `stack_slot #25+68`, source type `i16`, and destination type
`i32`. The current object-emission diagnostic owner is the generic
`unsupported_move_bundle_target_shape` path in
`fragment_for_prepared_move_bundle`, which reports
`fragment_status=generic_move_bundle_materialization_failed`. The semantic
owner for Step 3 should be the producer/classification side of the prepared
move reason or a producer-owned diagnostic: either publish a distinct
conversion-aware register-source stack-destination contract for this
`sext i16 -> i32` store-publication shape, or reject it before RV64
materialization with a precise owner-specific diagnostic.

Adjacent shapes that must remain fail-closed:

- missing source authority: no source home, non-register source home for
  `consumer_register_to_stack`, unknown source type/size, or source facts only
  inferable from names/ABI order
- missing destination authority: no destination home, non-stack-slot
  destination home, missing/large destination stack offset, destination
  override offset, or unknown destination scalar type/size
- unsupported register bank: source home or storage plan is not RV64 GPR, or
  the target-register identity conflicts with RV64 GPR/general register facts
- unsupported width: destination contiguous width is not 1, source and
  destination scalar sizes differ, or the scalar/store size is not supported by
  the RV64 load/store funct3 helper
- contradictory storage facts: source storage plan does not agree with the
  explicit source GPR, destination storage plan does not agree with a coherent
  GPR frame slot where required, or value-home and storage-plan facts disagree
- producer misclassification: register-source stack-destination moves tagged
  as `consumer_stack_to_stack`, stack-source moves tagged as
  `consumer_register_to_stack`, conversion moves without a distinct
  conversion-aware reason/authority, or any shape requiring inference from row
  names/local names/raw BIR text
- idea 516 multi-source regressions: non-parallel multi-source
  stack-destination bundles must continue to reject at
  `prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination`
  before RV64 object emission attempts materialization

## Suggested Next

Execute Step 3 by either adding a producer/classification repair that publishes
or rejects the `pr27073.c` conversion-aware register-source stack-destination
shape, or by keeping RV64 object emission fail-closed with a more precise
owner diagnostic. Do not broaden the existing same-width
`consumer_register_to_stack` materializer.

## Watchouts

Do not make `consumer_stack_to_stack` accept register sources as a shortcut:
that would blur idea 513's stack-to-stack contract and hide the producer
misclassification in `pr27073.c`.

Do not make the existing `consumer_register_to_stack` path silently accept
`i16 -> i32` by choosing either the source size or destination size ad hoc. The
conversion requires an explicit contract because `sh a4, 68(sp)` would store
the wrong destination width and `sw a4, 68(sp)` would store an unextended
source value unless the producer proves the register already holds the widened
value.

Keep `20010518-1.c` as the idea 516 guard. Its current rejection is still the
producer/classifier category
`ambiguous_non_parallel_multi_source_stack_destination`, not the remaining
idea 514 single-move boundary.

## Proof

Evidence-only packet; no implementation or test changes were made, and the
delegation explicitly prohibited touching `test_before.log` or
`test_after.log`.

Commands run:

- `./build/c4cll --dump-prepared-bir --target riscv64-unknown-linux-gnu --mir-focus-function foo tests/c/external/gcc_torture/src/pr27073.c`
- `./build/c4cll --codegen obj --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/pr27073.c -o /tmp/c4c_514_pr27073.o`
- `./build/c4cll --codegen obj --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/20010518-1.c -o /tmp/c4c_514_20010518.o`
- `git diff --check -- todo.md`

The two object commands intentionally fail as probes: `pr27073.c` remains at
`unsupported_move_bundle_target_shape`, and `20010518-1.c` remains at
`prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination`.
