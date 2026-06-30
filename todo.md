Status: Active
Source Idea Path: ideas/open/451_stack_home_branch_operand_materialization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Stack-Home Branch Materialization Contract

# Current Packet

## Just Finished

Completed Step 2 contract definition for idea 451.

Decision: do not select an RV64 consumer implementation packet yet. The
stack-home branch materialization contract is definable, but current prepared
facts are insufficient to satisfy it. Existing evidence exposes stack object
identity, value ids, slot ids, offsets, object size/alignment, and branch
operand roles; it does not expose branch-site stack-load policy, value
freshness proof, or clobber-safety proof for loading stack-slot homes as branch
operands or conditions.

Accepted contract facts:

| Area | Required fact |
| --- | --- |
| Branch identity | Prepared `branch_condition` for the terminator block with matching condition, predicate, compare type, labels, and operand role. |
| Stack object identity | `PreparedValueHome` maps the value to a stack slot with function name, value id, slot id, offset, width, and alignment. |
| Slot extent | Matching stack-layout object/frame slot exists and is large/aligned enough for the value type. |
| Load policy | Prepared metadata explicitly authorizes a branch-site stack load for the condition/lhs/rhs role and width. |
| Freshness | Prepared metadata proves the stack slot contains the current value at the branch site. |
| Clobber safety | Prepared metadata proves no intervening call, helper, publication move, or stack write clobbers the slot before the branch-site load. |
| Scratch safety | Consumer policy proves legal load scratch registers/order without clobbering still-needed operands. |
| Pointer provenance | Pointer operands must satisfy explicit prepared pointer authority or a separately named opaque policy; unknown-compatible pointer-value memory is not enough. |

Representative status:

- `f.block_1` is structurally closest but not accepted: `%t1/%t2` have stack
  homes and `%p.reg2` is GPR, but there is no load/freshness/clobber authority.
- `f.block_4` is rejected for a first consumer packet because `%t7` additionally
  derives from pointer-value memory with `layout_authority=unknown` and
  `range_verdict=unknown_compatible`.
- `f.logic.end.14` is rejected for this first pointer-relational packet because
  `%t22` is a select-result stack destination with block-entry publication
  `unsupported_destination_storage`.

Exact blocker/owner: producer/prepared branch-stack-load authority metadata.
The needed owner must publish or reject records tying a prepared branch
condition to each stack-home condition/operand value, stack slot object, value
width/alignment, branch-site load policy, freshness proof, clobber-safety proof,
and pointer-provenance status.

Artifact:
`build/agent_state/451_step2_stack_home_branch_contract/contract.md`.

## Suggested Next

Execute Step 3 from `plan.md`: route the first materialization packet as
blocked on producer/prepared metadata, unless the supervisor chooses to split a
new source idea immediately.

Suggested artifact directory:
`build/agent_state/451_step3_stack_home_branch_materialization_route/`.

Step 3 should not edit RV64 lowering from the current facts. It should record
the required producer/prepared metadata owner and required focused tests for
branch-stack-load authority, or activate/split that owner through the
plan-owner flow.

## Watchouts

- Do not select RV64 implementation from stack-slot homes alone.
- Do not weaken existing GPR-compatible branch publication predicates.
- Do not infer stack-home values, freshness, loads, operands, or conditions
  from raw BIR shape, stack-slot spelling, block order, filenames, function
  names, or one prepared dump layout.
- Treat stack object identity and slot offsets as necessary but not sufficient.
- Keep pointer-value/provenance publication for `%t7`, select-result/block-entry
  stack destination work for `%t22/%t23`, instruction/storage owners, and
  unrelated residuals separate.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 2 proof:

```sh
git diff --check
```

Result: passed.
