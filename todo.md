Status: Active
Source Idea Path: ideas/open/457_before_instruction_stack_to_register_move_materialization.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Completed Step 4 residual disposition for idea 457. Evidence is recorded under
`build/agent_state/457_step4_residual_disposition/disposition.md`.

Disposition: idea 457 is complete for audit/contract/disposition but blocked
for implementation. There is no exact remaining RV64 move-bundle packet that is
safe with the current prepared facts.

Residual owner:

- `20010329-1`
- `move_bundle phase=before_instruction authority=none block_index=4
  instruction_index=2`
- paired register-destination moves into `%t18`/`t0`
- source producer `%t18 = bir.ule ptr %t15, %t17`
- select carrier `%t22 = bir.select uge ptr %t5, %t7, i32 %t18, 0`
- edge transfer `logic.rhs.end.13 -> logic.end.14 incoming=%t18
  destination=%t22`

The missing fact is producer/prepared placement authority for this bundle. The
object route cannot tell whether the bundle should emit at the join
instruction, be suppressed because predecessor-edge publication already
rematerializes the source producer, or carry predecessor/successor edge
identity as edge-owned materialization. Same-block emission can overwrite the
false-edge selected value; sequential ordinary moves are unsafe because both
moves target `%t18`/`t0`.

Lifecycle recommendation: plan owner should close idea 457 as blocked/split by
producer metadata and open or activate a follow-up for select-edge source
producer move-bundle placement authority before any RV64 consumer work resumes.

## Suggested Next

Plan-owner close/split review. The durable next initiative should publish one
of:

- explicit before-instruction register-destination bundle placement semantics
  for select-edge source producers;
- predecessor/successor edge identity on edge-source producer dependency
  bundles; or
- precise suppression authority when the predecessor-edge publication already
  consumes the source producer.

After that producer metadata exists, a later RV64 consumer idea can revisit
before-instruction register-destination move materialization. Do not implement
RV64 lowering before the placement/edge authority exists.

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

Step 4 disposition-only validation:

```sh
git diff --check
```

Result: passed.
