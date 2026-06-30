Status: Active
Source Idea Path: ideas/open/470_branch_stack_load_policy_freshness.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Branch-Site Policy Freshness Inputs

# Current Packet

## Just Finished

Completed Step 1 audit for idea 470. Current prepared records expose the
branch-stack-load rows, but no real row has target-consumable
`load_from_stack_slot` authority because policy, freshness, and clobber-safety
facts are not produced.

Representative audit:

| Row | Current record | Policy | Freshness | Clobber safety | Boundary | First owner |
| --- | --- | --- | --- | --- | --- | --- |
| `f.block_1` condition `%t2` | `unsupported_terminator`, `pointer_status=not_pointer` | Not reachable yet | Missing | Missing | Scalar condition | Branch-site relationship acceptance before policy. |
| `f.block_1` lhs `%t1` | `unsupported_terminator`, `pointer_status=unknown` | Not reachable yet | Missing | Missing | Pointer status unknown | Branch-site relationship first; pointer/provenance separate. |
| `f.block_4` condition `%t8` | `unsupported_terminator`, `pointer_status=not_pointer` | Not reachable yet | Missing | Missing | Scalar condition paired with pointer lhs | Branch-site relationship first. |
| `f.block_4` lhs `%t7` | `unsupported_terminator`, `pointer_status=unknown` | Not reachable yet | Missing | Missing | Pointer-value/provenance | Pointer-value/provenance remains first owner before target consumption. |
| `f.logic.end.14` condition `%t23` | `value=%t23`, `value_id=17`, `slot=#21`, `object=#21`, `status=missing_policy` | Missing `load_from_stack_slot` | Missing | Missing | Scalar condition | First bounded policy/freshness/clobber candidate. |
| `f.logic.end.14` lhs `%t22` | `value=%t22`, `value_id=16`, `slot=#20`, `object=#20`, `status=missing_policy` | Missing | Missing | Missing | Select-result stack destination | Keep fail-closed; select-result owner remains separate. |

First exact Step 2 target: define the policy/freshness/clobber contract for
scalar condition records already populated through branch/value/home/frame-slot
facts, represented by `f.logic.end.14` condition `%t23`. Preserve
`unsupported_terminator`, pointer-status unknown, pointer-value provenance, and
select-result stack-destination rows as rejected/fail-closed shapes unless a
separate prerequisite packet proves them.

Artifact:
`build/agent_state/470_step1_policy_freshness_audit/audit.md`.

## Suggested Next

Execute Step 2: Define Policy Freshness Contract. Record accepted scalar
condition facts, rejected adjacent shapes, target files/tests for a bounded
producer packet if justified, and whether the `unsupported_terminator`
branch-site relationship gap is a prerequisite packet or remains an
unavailable status.

## Watchouts

- Do not edit implementation files during Step 2 unless explicitly redirected.
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
