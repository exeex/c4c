Status: Active
Source Idea Path: ideas/open/579_rv64_prepared_stack_destination_move_bundle_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Move-Bundle Authority Coverage

# Current Packet

## Just Finished

Completed Step 1 localization for
`ideas/open/579_rv64_prepared_stack_destination_move_bundle_authority.md`
without implementation changes.

Fresh reproduction under
`build/agent_state/579_rv64_prepared_stack_destination_move_bundle_authority/step1/src_20000605-1.c/`
confirms the representative RV64 object route still rejects with:

```text
prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination:
prepared move-bundle classifier rejected ambiguous non-parallel multi-source stack-destination authority
```

Exact first rejected move-bundle authority shape from
`dump-prepared-bir.txt`:

- Owner/source route: `render_image_rgb_a`, block `for.cond.2`
  (`block_index=1`), `BeforeInstructionCopies` for instruction index `2`.
- Owner instruction: `%t25 = bir.slt i32 %t23, %t24`; `prepared-control-flow`
  records `%t25` as the fused branch condition for `for.cond.2`.
- Bundle: `phase=before_instruction`, `authority=none`, no parallel-copy
  bundle, `move_count=2`.
- Move 0: `from_value_id=22` (`%t23`, register `t0`, loaded from `%lv.y` /
  frame slot `#0`) to `to_value_id=24`.
- Move 1: `from_value_id=23` (`%t24`, register `s2`, loaded from `%lv.ye` /
  frame slot `#1`) to `to_value_id=24`.
- Destination: `%t25`, `value_id=24`, `kind=stack_slot`, `slot_id=#12`,
  stack offset `36`, size/alignment `4`, object `#12` with
  `source_kind=regalloc.spill_slot`.
- Classifier branch:
  `prepared_move_bundle_has_ambiguous_multi_source_stack_destination(...)`
  returns true for a non-parallel `BeforeInstruction` bundle with
  `authority=none`, multiple register sources, and the same stack destination;
  `prepared_move_bundle_is_select_materialization_stack_destination(...)` does
  not apply because the owner is `slt`, not `select`.

Classification: this first source pair looks genuinely ambiguous/invalid for
the current non-parallel stack-destination authority. The two sources are both
required by the same compare instruction, are not mutually exclusive, are not
an ordered copy sequence to distinct destinations, and have no select or
parallel-copy authority that explains why both should write the single result
spill slot before the compare.

## Suggested Next

Delegate Step 2 to add focused coverage for the exact invalid authority shape:
a non-select `BeforeInstruction` binary/compare consumer with two register
sources targeting the same stack-homed result and no parallel-copy authority
must remain fail-closed with
`AmbiguousNonParallelMultiSourceStackDestination`.

Primary test surface:
`tests/backend/mir/backend_riscv_object_emission_test.cpp`, near the existing
register-source stack-destination and mixed stack-destination move-bundle
coverage around `builds_prepared_before_instruction_register_to_stack...` /
`builds_prepared_mixed_stack_destination_move_bundle_object`, plus the existing
select-publication stack-destination coverage for the valid mutually-exclusive
shape.

## Watchouts

- The 574 FP binary acceptance criteria are complete; do not fold this
  move-bundle authority work back into FP binary lowering.
- Do not bypass the prepared move-bundle classifier broadly. The representative
  first blocker is currently a genuine ambiguity in prepared authority, not a
  valid ordered stack-destination move sequence.
- Step 2 should keep the valid stack-destination surface tied to select or
  parallel-copy authority, not to `src/20000605-1.c`, `render_image_rgb_a`, or
  `%t25`.
- Step 3 implementation surface is split between the classifier in
  `src/backend/prealloc/prepared_object_traversal.cpp` and RV64 diagnostic /
  materialization handling in
  `src/backend/mir/riscv/codegen/object_emission.cpp`; if the desired repair is
  producer-side, the prepared move-bundle producer for binary/compare
  consumers must stop publishing multi-source writes to one stack result slot.
- Keep filename/function/value-name matching out of the route.

## Proof

No code-change proof required for this localization packet. Commands run:

```sh
build/c4cll -I /workspaces/c4c --codegen obj --target riscv64-linux-gnu \
  tests/c/external/gcc_torture/src/20000605-1.c \
  -o build/agent_state/579_rv64_prepared_stack_destination_move_bundle_authority/step1/src_20000605-1.c/object-route.o
```

Result: exit `2`, reproduced
`ambiguous_non_parallel_multi_source_stack_destination`; log path
`build/agent_state/579_rv64_prepared_stack_destination_move_bundle_authority/step1/src_20000605-1.c/object-route.log`.

```sh
build/c4cll -I /workspaces/c4c --dump-prepared-bir --target riscv64-linux-gnu \
  tests/c/external/gcc_torture/src/20000605-1.c
```

Result: exit `0`; dump path
`build/agent_state/579_rv64_prepared_stack_destination_move_bundle_authority/step1/src_20000605-1.c/dump-prepared-bir.txt`.
No broad backend CTest was run, and no `test_after.log` was produced because
this was a non-code localization packet.
