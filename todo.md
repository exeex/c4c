Status: Active
Source Idea Path: ideas/open/471_branch_site_stack_slot_freshness_clobber_safety_metadata.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Completed Step 4 residual disposition for idea 471. The plan is close-ready as
a negative producer-feasibility result: the carrier and planner are adequate,
but current prepared inputs still cannot prove branch-site stack-slot
current-value or no-clobber certificates for scalar condition rows.

Residual classification:

| Row | Current disposition | First owner |
| --- | --- |
| `f.logic.end.14` condition `%t23`, slot `#21` | Identity facts exist, but no durable fact proves the slot contains current `%t23` at the branch site or remains unclobbered. | Split/narrow producer for branch-site current-value plus no-clobber certificate. |
| `f.logic.end.14` lhs `%t22`, slot `#20` | Select-result stack-destination/materialization remains first. | Select-result / block-entry stack-destination owner. |
| `f.block_1` `%t2` and `f.block_4` `%t8` | Still `unsupported_terminator`; not eligible for freshness/clobber policy. | Branch-site relationship acceptance owner. |
| `f.block_1` `%t1` and `f.block_4` `%t7` | Pointer/provenance plus branch-site relationship boundaries remain. | Pointer-value provenance and branch-site relationship owners. |

Lifecycle recommendation: close idea 471 by split/route. Activate a narrower
producer initiative only for branch-site stack-slot current-value and
no-clobber certificates. RV64 branch-load consumption remains blocked until
available `PreparedBranchStackLoadAuthority` records exist.

Preserved boundaries: no RV64 lowering, no consumption of unavailable records,
no raw-shape inference, no select-result/pointer/unsupported-terminator
widening, and no baseline/review/log churn.

Artifact:
`build/agent_state/471_step4_residual_disposition/disposition.md`.

## Suggested Next

Plan-owner should close idea 471 or split/activate a new narrow producer idea:
`branch_site_stack_slot_current_value_no_clobber_certificate`.

Required future facts:

- current-value source identity for the stack slot at the branch site;
- path/dominance validity from source to branch site;
- stack-write exclusion for the same frame slot/object;
- call/helper/inline-asm clobber modeling;
- publication, move-bundle, and parallel-copy non-clobber proof;
- focused positive and fail-closed tests before any RV64 consumer work resumes.

Proof:

```sh
git diff --check
```

## Watchouts

- Do not set freshness/clobber-safety from raw BIR adjacency, stack homes, or
  object identity alone.
- Step 3 must block if current-value source, path validity, stack-write
  summary, call/helper effects, or move/publication clobber facts are missing.
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

Step 4 proof:

```sh
git diff --check
```

Result: passed.
