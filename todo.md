Status: Active
Source Idea Path: ideas/open/475_prepared_frame_slot_source_fact_population.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Real Source-Fact Population Inputs

# Current Packet

## Just Finished

Completed Step 1 audit for idea 475. Existing prepared evidence can populate
target identity for the `%t23` scalar condition row, but it still cannot
populate an available real source fact for slot `#21`.

Representative source-fact population table:

| Row/family | Existing usable inputs | Rejected / missing inputs | Classification |
| --- | --- | --- | --- |
| `f.logic.end.14` condition `%t23`, slot `#21` | `branch_condition logic.end.14 ... condition=%t23 compare=ne i32 %t22, 0`; `home %t23 value_id=17 kind=stack_slot slot_id=21 offset=156`; `object #21 ... type=i32 size=4 align=4`; `branch_stack_load_authority ... role=condition value=%t23 ... status=missing_policy slot=#21 object=#21`. | No explicit write/materialization event for the `%t23 = ne i32 %t22, 0` result into slot `#21`; no path/dominance proof; no same-slot write exclusion; no call/helper/inline-asm effect-safety proof; no publication/move/parallel-copy non-clobber classification. | In-scope scalar candidate, but current real status remains `missing_materialization_event`. |
| Nearby `%t23` move bundle | `move_bundle phase=before_instruction authority=none block_index=11 instruction_index=3` has `move from_value_id=16 to_value_id=17 destination_storage=stack_slot reason=consumer_stack_to_stack`. | This is a plain value copy from `%t22` to `%t23` storage, not a semantic compare-result materialization of `%t23`; authority is `none`; source value would mismatch the required `%t23` current-value claim. | Rejected as population evidence; would be `materialization_value_mismatch` or missing semantic materialization if fed to the carrier. |
| `%t22` select-result stack destination, slot `#20` | `select_chain ... value=%t22 root_is_select=yes`; block-entry publications for `%t22` exist but are `unsupported_destination_storage`; branch-stack-load row has `role=lhs value=%t22 ... status=missing_policy`. | Select-result/block-entry stack-destination publication is outside idea 475; no source-fact population should treat it as ordinary `%t23` materialization. | Protected `unsupported_boundary`; separate select-result/block-entry owner. |
| `%t1` / `%t7` pointer/provenance rows | Stack homes and branch conditions exist for pointer relational branches. | Pointer status/provenance remains unknown or pointer-value derived; idea 475 must not solve pointer provenance through stack-slot source facts. | Protected `unsupported_boundary`; separate pointer/provenance owner. |
| `%t2` / `%t8` unsupported-terminator relationship rows | Branch-stack-load records exist with durable unavailable status. | Branch-site relationship is still `unsupported_terminator`; source-fact population must not accept these rows. | Protected `unsupported_boundary`; separate branch-site relationship / terminator owner. |

First exact Step 2 target: define the real source-fact population contract for
the scalar `%t23` row. It must say which prepared records can establish a
semantic compare-result materialization/write into slot `#21`, path validity,
same-slot write exclusion, and effect non-clobber facts, and it must reject the
current `authority=none` stack-to-stack copy as insufficient.

Artifacts:

- `build/agent_state/475_step1_source_fact_population_audit/audit.md`

## Suggested Next

Execute Step 2 from `plan.md`: Define Population Contract. The contract should
decide whether a bounded producer packet exists for semantic scalar compare
materialization into a frame slot, or whether the exact blocker is missing
instruction-result materialization/write and interval no-clobber metadata.

## Watchouts

- Do not mark downstream `PreparedBranchStackLoadAuthority` available in idea
  475.
- Do not infer `%t23` materialization from stack homes, final storage, raw BIR
  adjacency, value names, block names, testcase shape, or dump order.
- The `from_value_id=16 -> to_value_id=17` stack move is not proof of the
  `%t23 = ne i32 %t22, 0` result.
- Keep `%t22` select-result stack-destination, `%t1/%t7` pointer/provenance,
  and `%t2/%t8` unsupported-terminator rows separate.

## Proof

Delegated proof:

```sh
git diff --check
```

Result: passed.
