Status: Active
Source Idea Path: ideas/open/472_branch_site_stack_slot_current_value_no_clobber_certificate.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Current-Value No-Clobber Certificate Inputs

# Current Packet

## Just Finished

Completed Step 1 audit for idea 472. The representative `%t23` scalar
condition row has branch/value/home/frame-slot/object identity, but current
prepared evidence still lacks a producer-owned current-value/no-clobber
certificate for slot `#21` at `f.logic.end.14`.

Representative audit:

| Row | Source identity | Path/dominance | Stack-write effects | Call/helper/inline-asm effects | Publication/move/parallel-copy effects | Classification |
| --- | --- | --- | --- | --- | --- | --- |
| `f.logic.end.14` condition `%t23`, slot `#21` | Branch condition `ne i32 %t22, 0`; `value_id=17`; home slot `#21`; object `#21` type `i32` size `4` align `4`; raw BIR computes `%t23` before branch. | Control-flow reaches `logic.end.14` from two predecessors, but no prepared certificate proves the slot current-value source dominates all paths. | No prepared write/materialization row for current `%t23` into slot `#21`; no same-slot write exclusion. | No relevant call is shown before this branch, but no modeled effect certificate is tied to slot `#21`. | `%t22` publications/parallel copies target slot `#20`; no non-clobber certificate exists for slot `#21`. | In-scope Step 2 contract target; not consumable. |
| `f.logic.end.14` lhs `%t22`, slot `#20` | Select-chain and value-home facts exist. | Predecessor-edge transfers exist. | Current-value proof blocked by select-result stack destination. | Separate boundary. | Block-entry publications are `unsupported_destination_storage`. | Fail closed: select-result/block-entry stack-destination owner. |
| `f.block_1` `%t2` / `%t1` | Branch rows exist but authority rows are `unsupported_terminator`; `%t1` also pointer-status unknown. | Not eligible. | Not reached. | Not reached. | Not reached. | Fail closed: branch-site relationship plus pointer/provenance for `%t1`. |
| `f.block_4` `%t8` / `%t7` | Branch rows exist but authority rows are `unsupported_terminator`; `%t7` also pointer-status unknown. | Not eligible. | Not reached. | Not reached. | Not reached. | Fail closed: branch-site relationship plus pointer/provenance for `%t7`. |

First Step 2 target: define the certificate contract for scalar condition rows
like `%t23`, including explicit current-value source, materialization/write to
the exact frame slot, path validity, same-slot write exclusion,
call/helper/inline-asm effect policy, and publication/move-bundle/
parallel-copy non-clobber proof. Raw BIR adjacency, stack homes, object ids,
function names, testcase shape, and dump order remain rejected.

Artifact:
`build/agent_state/472_step1_current_value_no_clobber_audit/audit.md`.

## Suggested Next

Execute Step 2 from `plan.md`: define the current-value/no-clobber certificate
contract for scalar branch condition stack slots.

Suggested artifact directory:
`build/agent_state/472_step2_current_value_no_clobber_contract/`.

## Watchouts

- Do not edit implementation files during Step 2 unless explicitly delegated.
- Do not implement RV64 branch-load emission in this producer plan.
- Do not accept unavailable `PreparedBranchStackLoadAuthority` records as
  target authority.
- Do not infer current value or no-clobber safety from stack homes, offsets,
  object ids, raw BIR, block labels, function names, testcase names, or dump
  order.
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
