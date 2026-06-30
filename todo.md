Status: Active
Source Idea Path: ideas/open/473_branch_site_stack_slot_materialization_no_clobber_source.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Materialization No-Clobber Source Inputs

# Current Packet

## Just Finished

Completed Step 1 audit for idea 473. The representative `%t23` row remains an
in-scope source-fact target, but current prepared evidence still lacks an
explicit frame-slot materialization/write of current `%t23` into slot `#21`
and lacks no-clobber source facts for that slot.

Source-fact audit:

| Row | Source value | Materialization/write inputs | Path/dominance | Same-slot writes | Call/helper/inline-asm | Publications/moves/parallel copies | Classification |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `f.logic.end.14` condition `%t23`, slot `#21` | Branch condition names `%t23`; raw BIR computes `%t23 = ne i32 %t22, 0`; home/storage slot `#21`. | No prepared row states `%t23` was written/materialized into slot `#21`; home/storage identity is not a write event. | CFG shows predecessors into `logic.end.14`, but no materialization dominates or is path-valid by source fact. | Later local stores target other parameter slots; no slot `#21` write exclusion summary exists. | Later `abort` callsite exists; no effect safety fact is tied to slot `#21`. | `%t22` publications/parallel copies target slot `#20`; no slot `#21` non-clobber fact. | In-scope Step 2 contract target; producer not justified yet. |
| `f.logic.end.14` lhs `%t22`, slot `#20` | Select-result source exists. | Block-entry publications/parallel copies target `%t22`, but destination storage is unsupported. | Edge-transfer data exists. | Select-result stack destination is first owner. | Separate boundary. | Parallel copies target `%t22`. | Fail closed: select-result/block-entry stack-destination owner. |
| `f.block_1` `%t2` / `%t1` | Branch rows exist, but authority rows are `unsupported_terminator`; `%t1` pointer status unknown. | Not eligible. | Not reached. | Not reached. | Not reached. | Not reached. | Fail closed: branch-site relationship plus pointer/provenance for `%t1`. |
| `f.block_4` `%t8` / `%t7` | Branch rows exist, but authority rows are `unsupported_terminator`; `%t7` pointer status unknown. | Not eligible. | Not reached. | Not reached. | Not reached. | Not reached. | Fail closed: branch-site relationship plus pointer/provenance for `%t7`. |

First Step 2 target: define source-fact records/statuses for scalar branch
condition slots, including exact source value identity, explicit frame-slot
materialization/write, path validity, same-slot write exclusion,
call/helper/inline-asm effect safety, and publication/move-bundle/
parallel-copy non-clobber proof.

Artifact:
`build/agent_state/473_step1_materialization_no_clobber_audit/audit.md`.

## Suggested Next

Execute Step 2 from `plan.md`: define the source-fact contract for
materialization/current-value and no-clobber evidence.

Suggested artifact directory:
`build/agent_state/473_step2_materialization_no_clobber_contract/`.

## Watchouts

- Do not edit implementation files during Step 2 unless explicitly delegated.
- Do not implement RV64 branch-load emission in this producer plan.
- Do not directly populate available branch-stack-load authority records.
- Do not infer source facts from stack homes, offsets, object ids, raw BIR,
  value names, block labels, function names, testcase names, or dump order.
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
