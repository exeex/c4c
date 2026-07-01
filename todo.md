Status: Active
Source Idea Path: ideas/open/503_rv64_before_return_prepared_move_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Before-Return Move Surfaces

# Current Packet

## Just Finished

Step 1 inspected the prepared before-return move-bundle publication and RV64
consumption surfaces for `return_stack_to_register` without implementation
changes.

Durable evidence:

- `build/agent_state/503_step1_before_return_move_surfaces/summary.md`
- `build/agent_state/503_step1_before_return_move_surfaces/representative_rows.tsv`

Representative row from idea 495:

- 1 row, `src/20080719-1.c`;
- event kind `pre_terminator_copies`;
- phase `before_return`;
- authority `none`;
- no parallel-copy bundle;
- destination storage `register`;
- move reason `return_stack_to_register`;
- diagnostic payload includes `destination_kind=function_return_abi`,
  `from_value_id=2`, `to_value_id=2`, and `op_kind=move`.

Prepared records/helpers to consume:

- `PreparedMoveBundle` for before-return route coordinates: function name,
  phase `BeforeReturn`, block index, instruction index, authority `None`, and
  move list.
- `PreparedMoveResolution` for `from_value_id`, `to_value_id`, destination
  kind `FunctionReturnAbi`, destination storage `Register`, destination
  register name/placement, op kind, and reason `return_stack_to_register`.
- `PreparedValueHome` plus `PreparedStackLayout` for proving the source value
  is a prepared scalar stack-slot home with a concrete frame slot offset.
- `PreparedMoveBundleLookups::before_return_abi_moves_by_source_and_bank` and
  `find_prepared_before_return_abi_move_by_source_and_destination_bank` for
  existing before-return ABI lookup semantics.
- `append_before_return_move_events` and
  `classify_prepared_object_move_bundle_consumer` for routing
  `BeforeReturn` bundles as non-parallel-copy `PreTerminatorCopies`.

RV64 hooks to extend:

- `fragment_for_prepared_move_bundle` is the direct event-loop hook for the
  representative `PreTerminatorCopies` move bundle.
- Existing return terminator emission already uses
  `find_prepared_before_return_abi_move_by_source_and_destination_bank` for
  FPR before-return moves and direct-global GPR returns, but it does not
  handle the generic scalar stack-to-GPR return move.
- Existing RV64 helpers available for Step 2 include
  `prepared_value_home_for_id`, `prepared_bir_value_type_for_name`,
  `rv64_scalar_memory_size_for_type`, `prepared_stack_slot_home_offset`,
  `rv64_register_number`, and `append_rv64_load_stack_to_register`.

Current prepared facts are complete enough for a narrow Step 2 implementation
of only `return_stack_to_register`: require non-parallel-copy
`PreTerminatorCopies`, phase `BeforeReturn`, authority `None`,
`FunctionReturnAbi` destination, GPR destination register/placement, stack-slot
source home/layout, scalar load size, and no cycle/temp/step/immediate/wide
register complications.

## Suggested Next

Execute Step 2 as a narrow implementation packet for only coherent
`return_stack_to_register` before-return moves. Materialize a prepared stack
slot load into the prepared return ABI GPR, and keep before-instruction,
out-of-SSA, select-publication, producer repairs, expectation rewrites,
pass/fail accounting, and baseline churn out of scope.

## Watchouts

- Do not infer return storage from testcase shape, source names, raw BIR order,
  final homes, target register conventions, or object output.
- Keep adjacent fail-closed coverage for missing source home, non-stack source,
  non-GPR destination bank, missing destination register, unsupported source
  type/size, destination kind confusion, before-instruction phase,
  out-of-SSA authority, and select-publication evidence.
- Preserve existing untracked review artifacts and the rejected
  `test_baseline.new.log`.

## Proof

Step 1 proof:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed > test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed after the supervisor reran the lifecycle proof without
self-clobbering `test_after.log`. Output is preserved in `test_after.log`.
