Status: Active
Source Idea Path: ideas/open/471_branch_site_stack_slot_freshness_clobber_safety_metadata.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Branch-Site Freshness And Clobber Inputs

# Current Packet

## Just Finished

Completed Step 1 audit for idea 471. Current prepared evidence exposes
branch-stack-load records plus useful nearby inputs, but it still does not
prove that any branch stack slot contains the current branch value at the branch
site or that the slot is safe from clobber before a load.

Representative audit:

| Row | Current record | Inputs present | Freshness / clobber evidence | First owner |
| --- | --- | --- | --- | --- |
| `f.logic.end.14` condition `%t23` | `value_id=17`, `slot=#21`, `object=#21`, `status=missing_policy` | Stack home/object/frame slot, branch condition, no later call on same block, no move/publication targeting `%t23`. | Missing explicit slot-current-value proof and missing explicit no-clobber proof. | Step 2 contract target. |
| `f.logic.end.14` lhs `%t22` | `value_id=16`, `slot=#20`, `object=#20`, `status=missing_policy` | Select-chain and block-entry publication rows exist. | Missing; blocked by select-result stack-destination publication. | Select-result/block-entry owner first. |
| `f.block_1` condition `%t2` | `unsupported_terminator`, `pointer_status=not_pointer` | Stack home/object exists. | Not reachable because branch-site relationship is not accepted. | `unsupported_terminator` prerequisite. |
| `f.block_1` lhs `%t1` | `unsupported_terminator`, `pointer_status=unknown` | Stack home/object exists. | Not reachable. | `unsupported_terminator` plus pointer/provenance. |
| `f.block_4` condition `%t8` | `unsupported_terminator`, `pointer_status=not_pointer` | Stack home/object exists. | Not reachable. | `unsupported_terminator` prerequisite. |
| `f.block_4` lhs `%t7` | `unsupported_terminator`, `pointer_status=unknown` | Stack home/object exists. | Not reachable. | Pointer-value/provenance after branch-site relationship. |

First Step 2 target: define the freshness/clobber-safety contract for scalar
condition records that already have populated branch/value/home/frame-slot/
object facts, represented by `f.logic.end.14` condition `%t23`. Preserve
select-result, pointer/provenance, and `unsupported_terminator` rows as
separate/fail-closed owners.

Artifact:
`build/agent_state/471_step1_freshness_clobber_audit/audit.md`.

## Suggested Next

Execute Step 2: Define Freshness Clobber-Safety Contract. Specify exact facts
for stack-slot current-value proof, branch-site path/dominance, and no
intervening clobber; name implementation/test surfaces only if a bounded
producer packet is justified.

## Watchouts

- Do not edit implementation files during Step 1.
- Do not implement RV64 branch-load emission in this producer plan.
- Do not accept unavailable `PreparedBranchStackLoadAuthority` records as
  target authority.
- Do not infer freshness or clobber safety from stack homes, offsets, object
  ids, raw BIR, block labels, function names, testcase names, or dump order.
- Keep pointer-value/provenance, select-result stack-destination, and
  unsupported-terminator boundaries separate.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Classification proof:

```sh
git diff --check
```

Result: passed.
