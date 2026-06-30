Status: Active
Source Idea Path: ideas/open/457_before_instruction_stack_to_register_move_materialization.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route First Move-Bundle Packet

# Current Packet

## Just Finished

Completed Step 3 for idea 457 as a routed blocker, not an RV64 implementation.
Evidence is recorded under
`build/agent_state/457_step3_register_destination_move_materialization/blocker.md`.

The target bundle is:

- `move_bundle phase=before_instruction authority=none block_index=4
  instruction_index=2`
- `from_value_id=7 to_value_id=10 destination_storage=register
  reason=consumer_register_to_register`
- `from_value_id=9 to_value_id=10 destination_storage=register
  reason=consumer_stack_to_register`

The compare producer is `%t18 = bir.ule ptr %t15, %t17`, and `%t18` is an
incoming value for the select-materialization carrier `%t22`. The RHS `%t17`
has explicit `rematerialize_cast_from_source status=available` authority, while
the stack-load alternative remains `missing_stack_freshness`.

Blocker: the current prepared facts do not say whether this
before-instruction register-destination bundle should emit in the join block,
be suppressed because the predecessor-edge publication already rematerializes
the source producer, or be treated as an edge-owned materialization bundle. The
bundle has no printed `source_parallel_copy` identity, while the existing RV64
select-edge source producer path requires predecessor/successor labels to
consume edge publication intent. Emitting the bundle as same-block moves is
unsafe because both moves target `%t18`/`t0`; emitting the compare in the join
block is unsafe for the select carrier because it can overwrite the false-edge
selected value.

No implementation files were changed. The rejected shapes remain fail-closed:
generic stack-to-register moves, stale `load_from_stack_slot
missing_stack_freshness`, same-destination sequential copies, raw `inttoptr`,
successor-result copies, unsafe scratch/clobber cases, and unrelated move
bundles.

## Suggested Next

Execute Step 4: `Residual Disposition And Close Readiness`.

Classify idea 457 as blocked on producer/prepared placement metadata unless the
plan owner wants to keep it active for a precise metadata packet. The durable
next owner should publish one of:

- explicit before-instruction register-destination bundle placement semantics
  for select-edge source producers;
- predecessor/successor edge identity on edge-source producer dependency
  bundles; or
- precise suppression authority when the predecessor-edge publication already
  consumes the source producer.

Do not implement RV64 lowering until that authority exists.

Re-read:

- `ideas/open/457_before_instruction_stack_to_register_move_materialization.md`
- `build/agent_state/457_step2_register_destination_move_contract/contract.md`
- `build/agent_state/457_step3_register_destination_move_materialization/blocker.md`

Step 4 proof command if disposition-only:

```sh
git diff --check
```

If code/test files unexpectedly change, use:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Do not reopen explicit cast-dependency authority consumption closed by idea
  456.
- Do not consume `load_from_stack_slot missing_stack_freshness`.
- Do not infer materialization authority from raw BIR shape, filenames,
  function names, block names, testcase shape, value ids alone, or one
  prepared dump.
- Keep general stack-home branch consumer work routed to
  `ideas/open/451_stack_home_branch_operand_materialization.md`.
- Keep pointer-value provenance and generic instruction-side lowering out of
  this plan.
- Do not infer edge placement from value ids, block indexes, instruction
  indexes, or the representative testcase. The missing fact is producer-owned
  placement/source-producer authority.
- Do not suppress arbitrary before-instruction register-destination bundles
  without explicit authority.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, baseline logs, or `review/`.

## Proof

Step 3 route/blocker validation:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed. Proof log: `test_after.log`.
