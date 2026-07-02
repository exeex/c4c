Status: Active
Source Idea Path: ideas/open/514_rv64_register_source_stack_destination_move_bundles.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Rebuild Register-Source Move Evidence

# Current Packet

## Just Finished

Step 1 rebuilt the current RV64 object-route evidence for the two
representative rows without implementation edits.

Fresh probes:

- `build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20010518-1.c > /tmp/c4c_20010518-1.prepared`
- `build/c4cll --codegen obj --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20010518-1.c -o /tmp/c4c_20010518-1.o`
- `build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/pr27073.c > /tmp/c4c_pr27073.prepared`
- `build/c4cll --codegen obj --target riscv64-linux-gnu tests/c/external/gcc_torture/src/pr27073.c -o /tmp/c4c_pr27073.o`

Single-move representative, `src/pr27073.c`:

- Prepared bundle: function `foo`, `phase=before_instruction`,
  `block_index=0`, `instruction_index=1`, `authority=none`,
  `move_count=1`, `parallel_copy=no`.
- Move: `from_value_id=4` (`%p.count`) to `to_value_id=10` (`%t0`),
  `destination_kind=value`, `destination_storage=stack_slot`,
  `op_kind=move`, `reason=consumer_stack_to_stack`,
  `uses_cycle_temp_source=no`.
- Source home/storage: `%p.count value_id=4 kind=register reg=a4`;
  storage plan `encoding=register bank=gpr reg=a4 width=1 units=a4`.
  Source scalar fact is `i16` from the BIR signature and stack-layout object
  `#22`, `size=2`, `align=2`.
- Destination home/storage: `%t0 value_id=10 kind=stack_slot slot_id=25
  offset=68`; storage plan `encoding=frame_slot bank=gpr
  spill_slot=slot#25+stack68 width=1 slot_id=#25 stack_offset=68`.
  Destination scalar fact is `i32` from `%t0 = bir.sext i16 %p.count to i32`
  and stack-layout object `#25`, `size=4`, `align=4`.
- Rejection site: `src/backend/mir/riscv/codegen/object_emission.cpp`
  emits the exact diagnostic through
  `rv64_prepared_move_bundle_fragment_failure_diagnostic(...)`
  at the generic fragment failure tail. Current object-route stderr:
  `unsupported_move_bundle_target_shape: prepared move bundle requires unsupported RV64 moves event_kind=before_instruction_copies function=foo block_index=0 block_label=entry instruction_index=1 phase=before_instruction bundle_block_index=0 bundle_instruction_index=1 authority=none move_count=1 parallel_copy=no ... move[0].from_value_id=4 move[0].to_value_id=10 ... move[0].reason=consumer_stack_to_stack fragment_status=generic_move_bundle_materialization_failed`.
- RV64 authority assessment: source and destination locations are explicit
  enough to identify a register-source to stack-destination shape, but the
  source/destination scalar facts are not same-width (`i16` source, `i32`
  destination) and the move record carries no explicit extension semantics.
  RV64 should not materialize this as a plain store until Step 2 decides
  whether producer classification supplies enough authority or must
  reclassify the diagnostic.

Multi-move representative, `src/20010518-1.c`:

- Prepared bundle: function `add`, `phase=before_instruction`,
  `block_index=0`, `instruction_index=0`, `authority=none`,
  `move_count=2`, `parallel_copy=no`.
- Move 0: `from_value_id=0` (`%p.a`) to `to_value_id=13` (`%t0`),
  `destination_kind=value`, `destination_storage=stack_slot`,
  `op_kind=move`, `reason=consumer_stack_to_stack`,
  `uses_cycle_temp_source=no`.
- Move 1: `from_value_id=1` (`%p.b`) to `to_value_id=13` (`%t0`),
  `destination_kind=value`, `destination_storage=stack_slot`,
  `op_kind=move`, `reason=consumer_stack_to_stack`,
  `uses_cycle_temp_source=no`.
- Source homes/storage: `%p.a value_id=0 kind=register reg=a0` with
  `encoding=register bank=gpr reg=a0 width=1 units=a0`; `%p.b value_id=1
  kind=register reg=a1` with `encoding=register bank=gpr reg=a1 width=1
  units=a1`. Both source scalar facts are `i32`, `size=4`, `align=4`.
- Destination home/storage: `%t0 value_id=13 kind=stack_slot slot_id=8
  offset=32`; storage plan `encoding=frame_slot bank=gpr
  spill_slot=slot#8+stack32 width=1 slot_id=#8 stack_offset=32`.
  Destination scalar fact is `i32`, `size=4`, `align=4`.
- Rejection site: same
  `rv64_prepared_move_bundle_fragment_failure_diagnostic(...)` generic
  fragment failure path. Current object-route stderr:
  `unsupported_move_bundle_target_shape: prepared move bundle requires unsupported RV64 moves event_kind=before_instruction_copies function=add block_index=0 block_label=entry instruction_index=0 phase=before_instruction bundle_block_index=0 bundle_instruction_index=0 authority=none move_count=2 parallel_copy=no ... move[0].from_value_id=0 move[0].to_value_id=13 ... move[1].from_value_id=1 move[1].to_value_id=13 ... fragment_status=generic_move_bundle_materialization_failed`.
- RV64 authority assessment: the individual source and destination homes are
  explicit register-to-stack facts, and scalar/bank facts are coherent for
  `i32` GPR storage, but cardinality is not coherent for a minimal materializer:
  two moves target the same stack destination in one non-parallel bundle with
  no explicit combining, selection, or sequencing contract beyond order. RV64
  does not have enough authority to choose, merge, or drop one move.

## Suggested Next

Proceed to Step 2 by defining the accepted/rejected contract separately for
the single-move `pr27073.c` shape and the multi-move `20010518-1.c` shape.
The single-move case needs a decision on the `i16` source to `i32`
destination authority; the multi-move case should remain rejected unless a
producer-owned contract explains the two sources to one destination.

## Watchouts

The current `--codegen asm` path succeeds for both rows; the reproducing
failure is `--codegen obj`. Existing
`build/rv64_gcc_c_torture_backend/.../case.log` files are stale for this
packet and stop at `unsupported_param_home`, so they were not used as the
current rejection evidence.

## Proof

Evidence-only packet; no code-changing proof was required and no
`test_after.log` was requested. Ran the focused prepared/object probes listed
above and `git diff --check -- todo.md`.
