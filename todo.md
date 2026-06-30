Status: Active
Source Idea Path: ideas/open/451_stack_home_branch_operand_materialization.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route First Materialization Packet

# Current Packet

## Just Finished

Completed Step 3 routing packet for idea 451.

No RV64 materialization implementation is selected. The first materialization
packet is blocked on missing producer/prepared branch-stack-load authority
metadata. Current prepared facts show stack homes, slot ids, offsets, object
size/alignment, and branch conditions, but they do not prove that a stack slot
may be loaded at a branch site as the current condition/lhs/rhs value.

Exact blocker/owner: producer/prepared branch-stack-load authority metadata.

Required future record fields:

| Field group | Required content |
| --- | --- |
| Branch site | function, block, terminator coordinate, prepared branch-condition fields, predicate, compare type, labels |
| Operand role | condition/lhs/rhs, source value name/id, source type, GPR width |
| Stack home | stack-slot home, slot id, absolute stack offset, value size/alignment, stack-layout object/frame-slot match |
| Load policy | explicit permission to materialize this branch operand/condition by loading from the stack home |
| Freshness | proof that the stack slot contains the current SSA value at the branch site |
| Clobber safety | proof that no intervening store, helper, call, move bundle, publication, or stack write invalidates the slot before the branch load |
| Scratch/order | permitted scratch register/order constraints or enough metadata for the consumer to prove no operand clobber |
| Pointer status | proven pointer authority or separately named accepted opaque policy; unknown-compatible pointer-value memory remains rejected |
| Rejection status | explicit unavailable statuses for missing freshness/load policy, mismatched slot/object, pointer-provenance unknown, unsupported select-result stack destination, and raw-shape-only fixtures |

Required future tests:

- Positive coverage for branch lhs/condition stack homes accepted only with
  matching branch condition, stack object, width/alignment, load policy,
  freshness, and clobber-safety records.
- Negative coverage for stack homes without authority, stale/clobbered slots,
  mismatched branch/role/slot/width/object facts, pointer-value unknowns,
  select-result stack destinations, and raw-shape-only fixtures.
- Regression coverage that existing GPR-compatible branch routes remain
  unchanged.

Artifact:
`build/agent_state/451_step3_stack_home_branch_materialization_route/route.md`.

## Suggested Next

Execute Step 4 from `plan.md`: residual disposition and close-readiness review.

Suggested artifact directory:
`build/agent_state/451_step4_residual_disposition/`.

Step 4 should decide whether idea 451 closes by split/blocker after routing the
producer/prepared branch-stack-load authority owner, or remains active only if
the supervisor selects a precise in-scope metadata packet.

## Watchouts

- Do not select RV64 implementation from stack-slot homes alone.
- Do not weaken existing GPR-compatible branch publication predicates.
- Do not infer stack-home values, freshness, loads, operands, or conditions
  from raw BIR shape, stack-slot spelling, block order, filenames, function
  names, or one prepared dump layout.
- Treat stack object identity and slot offsets as necessary but not sufficient.
- The next implementation-capable owner is producer/prepared metadata, not
  RV64 lowering.
- Keep pointer-value/provenance publication for `%t7`, select-result/block-entry
  stack destination work for `%t22/%t23`, instruction/storage owners, and
  unrelated residuals separate.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 3 proof:

```sh
git diff --check
```

Result: passed.
