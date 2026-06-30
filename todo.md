Status: Active
Source Idea Path: ideas/open/470_branch_stack_load_policy_freshness.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Completed Step 4 residual disposition for idea 470 after Step 3 routed the
implementation packet. Idea 470 is not close-ready as an available-record
producer; it should close or retire by split to a narrower branch-site
stack-slot freshness/clobber-safety producer initiative.

Residual classification:

| Row | Current status | Residual owner |
| --- | --- |
| `f.logic.end.14` condition `%t23` | Populated `value_id=17`, `slot=#21`, `object=#21`, `status=missing_policy` | New producer metadata owner for branch-site `load_from_stack_slot`, freshness, and clobber safety. |
| `f.logic.end.14` lhs `%t22` | Populated `value_id=16`, `slot=#20`, `object=#20`, `status=missing_policy` | Select-result stack-destination owner before target consumption. |
| `f.block_1` condition `%t2` | `status=unsupported_terminator` | Branch-site relationship acceptance prerequisite before policy/freshness. |
| `f.block_1` lhs `%t1` | `status=unsupported_terminator`, `pointer_status=unknown` | Branch-site relationship plus pointer/provenance owner. |
| `f.block_4` condition `%t8` | `status=unsupported_terminator` | Branch-site relationship acceptance prerequisite. |
| `f.block_4` lhs `%t7` | `status=unsupported_terminator`, `pointer_status=unknown` | Pointer-value/provenance owner before branch-load availability. |

Close-readiness decision:

- The carrier and unavailable statuses are adequate.
- The missing owner is producer-owned branch-site stack-slot freshness and
  clobber-safety evidence.
- RV64 consumption must remain blocked until a later packet produces available
  branch-stack-load records.

Artifact:
`build/agent_state/470_step4_residual_disposition/disposition.md`.

## Suggested Next

Plan-owner lifecycle packet: split or activate a narrower producer idea for
branch-site stack-slot freshness and clobber-safety metadata, then close/retire
idea 470 as blocked by that first owner. Suggested follow-up owner:
`branch_site_stack_slot_freshness_clobber_safety_metadata`.

## Watchouts

- Do not edit RV64 target lowering.
- Do not set `load_from_stack_slot` unless freshness and clobber safety are
  also explicitly proven.
- Preserve `unsupported_terminator` rows as unavailable unless a separate
  prerequisite packet proves the branch-site relationship.
- The carrier capacity exists; the blocker is missing producer facts, not the
  enum/record shape.
- Do not implement RV64 branch-load emission in this producer plan.
- Do not accept unavailable `PreparedBranchStackLoadAuthority` records as
  target authority.
- Do not infer load authority from stack homes, offsets, object ids, raw BIR,
  block labels, function names, or testcase shape.
- Keep pointer-value/provenance and select-result stack-destination boundaries
  separate.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Classification proof:

```sh
git diff --check
```

Result: passed.
