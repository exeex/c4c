Status: Active
Source Idea Path: ideas/open/457_before_instruction_stack_to_register_move_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Before-Instruction Register-Destination Move Bundle

# Current Packet

## Just Finished

Completed Step 1 audit for idea 457. Evidence is recorded under
`build/agent_state/457_step1_before_instruction_register_destination_audit/`.

The inherited `20010329-1` probe still has `prepared_exit=0`, `object_exit=2`,
and `unsupported_move_bundle_target_shape: prepared move bundle requires
unsupported RV64 moves`. The residual owner is the later
`move_bundle phase=before_instruction authority=none block_index=4
instruction_index=2`, not the closed idea 456 cast-dependency consumer.

| Move | Source value | Destination value | Source home | Destination home | Reason | Carrier use | Scratch/clobber risk | Traversal event | First missing authority |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Register source | `from_value_id=7` / `%t15` | `to_value_id=10` / `%t18` | register `s1` | register `t0` | `consumer_register_to_register` | LHS operand for `%t18 = bir.ule ptr %t15, %t17` | Unsafe as a standalone sequential copy because the paired RHS move writes the same destination value/register | Prepared bundle is printed; current RV64 object route rejects the register-destination bundle shape | Explicit compare-operand materialization contract tying the bundle to binary operand roles plus non-clobbering register/order proof |
| Stack source | `from_value_id=9` / `%t17` | `to_value_id=10` / `%t18` | stack slot `#2+16`, with separate `rematerialize_cast_from_source status=available`; stack-load path is `missing_stack_freshness` | register `t0` | `consumer_stack_to_register` | RHS operand for `%t18 = bir.ule ptr %t15, %t17` | High if treated as ordinary stack-to-register: stale stack load is not authorized and writing `t0` can clobber the LHS operand | Same before-instruction register-destination bundle; idea 456 only exposed/suppressed the earlier authorized stack-destination publication | Authority to consume only the explicit cast-rematerialization path as compare-operand setup, while rejecting `load_from_stack_slot missing_stack_freshness` |

Bucket classification:

- Accepted ordinary register-destination moves: none. The two rows share
  destination value/register `%t18`/`t0`, so sequential move semantics are not
  safe.
- Candidate first packet: define and then implement only a prepared-fact-driven
  compare-operand materialization route for before-instruction
  register-destination bundles when source-producer, operand roles, homes, and
  scratch/order safety are explicit.
- Fail-closed: generic `consumer_stack_to_register`, raw stack loads,
  `load_from_stack_slot missing_stack_freshness`, same-destination bundles
  without compare-operand semantics, and successor-result copies.

## Suggested Next

Execute Step 2: `Define Register-Destination Move Materialization Contract`.
Define accepted and rejected before-instruction register-destination move
shapes for this family, especially the narrow compare-operand materialization
contract around the paired `%t15/%t17 -> %t18` bundle. The likely Step 3 packet
must consume prepared move-bundle facts, binary source-producer operand roles,
source/destination homes, explicit cast-rematerialization authority where
needed, and scratch/clobber safety. It must not be generic stack-to-register
support.

Re-read:

- `ideas/open/457_before_instruction_stack_to_register_move_materialization.md`
- `build/agent_state/457_step1_before_instruction_register_destination_audit/audit.md`
- `build/agent_state/457_step1_before_instruction_register_destination_audit/20010329-1.prepared.out`
- `build/agent_state/457_step1_before_instruction_register_destination_audit/20010329-1.object.err`

Record owned files/tests and the exact Step 3 proof command if a coherent
implementation packet exists:

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
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, baseline logs, or
  `review/`.

## Proof

Step 1 classification-only validation:

```sh
git diff --check
```

Result: passed.
