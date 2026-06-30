Status: Active
Source Idea Path: ideas/open/451_stack_home_branch_operand_materialization.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Completed Step 4 residual disposition for idea 451.

Decision: idea 451 is close-ready by split/blocker. The plan answered its core
question: current prepared stack-home branch facts are not sufficient to justify
RV64 branch operand/condition materialization. The exact missing owner is
producer/prepared branch-stack-load authority metadata.

Residual table:

| Area | Status | First owner |
| --- | --- | --- |
| Stack-home branch materialization | Blocked from RV64 implementation. Stack homes, slot ids, offsets, and object facts exist, but no branch-site load policy, freshness proof, or clobber-safety proof exists. | New producer/prepared branch-stack-load authority metadata owner. |
| `f.block_1` `%t2 = ult ptr %t1, %p.reg2` | Best future metadata candidate; not currently target-consumable. | Branch-stack-load authority metadata, with pointer/formal provenance caveat preserved. |
| `f.block_4` `%t8 = ult ptr %t7, %p.mr_HB` | Not first consumer row. `%t7` also depends on pointer-value memory with unknown layout/range authority. | Pointer-value/provenance owner plus branch-stack-load authority. |
| `f.logic.end.14` `%t23 = ne i32 %t22, 0` | Not first pointer-relational stack operand row. `%t22` is a select-result stack destination and block-entry publication is unsupported destination storage. | Select-result/block-entry stack-destination owner plus branch-stack-load authority. |
| GPR-compatible branch routes | Already owned/closed by earlier GPR-compatible pointer branch work. | No 451 action. |

Follow-up owner boundary: create a producer/prepared metadata surface that
publishes available or unavailable branch-stack-load authority records,
including branch-condition identity, operand role, stack slot/object facts,
load policy, freshness, clobber safety, scratch/order constraints, pointer
provenance status, and explicit unavailable statuses for rejected shapes.

Artifact:
`build/agent_state/451_step4_residual_disposition/disposition.md`.

## Suggested Next

Plan-owner close-readiness review for idea 451.

Recommended route: close idea 451 by split/blocker and, if continuing this
family, activate a new source idea for producer/prepared branch-stack-load
authority metadata. No further executor packet is selected inside idea 451.

## Watchouts

- Do not select RV64 implementation from stack-slot homes alone.
- Do not weaken existing GPR-compatible branch publication predicates.
- Do not infer stack-home values, freshness, loads, operands, or conditions
  from raw BIR shape, stack-slot spelling, block order, filenames, function
  names, or one prepared dump layout.
- Treat stack object identity and slot offsets as necessary but not sufficient.
- The next implementation-capable owner is producer/prepared branch-stack-load
  metadata, not RV64 lowering.
- Keep pointer-value/provenance publication for `%t7`, select-result/block-entry
  stack destination work for `%t22/%t23`, instruction/storage owners, and
  unrelated residuals separate.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 4 proof:

```sh
git diff --check
```

Result: passed.
