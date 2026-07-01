Status: Active
Source Idea Path: ideas/open/475_prepared_frame_slot_source_fact_population.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route Source-Fact Population

# Current Packet

## Just Finished

Completed Step 3 for idea 475 as a routing/blocker packet. No implementation
is selected for real source-fact population from current prepared evidence.

Exact blocker:

| Missing lower-level producer surface | Why it blocks `%t23` slot `#21` |
| --- | --- |
| Semantic instruction-result frame-slot materialization/write record | No prepared record states that `%t23 = ne i32 %t22, 0` is written/materialized into frame slot `#21`. |
| Path/dominance or edge-scope proof | No record proves a materialization/write reaches `f.logic.end.14` on the branch-consumer path. |
| Same-slot write exclusion | No interval fact proves slot `#21` is not overwritten between materialization and consumer. |
| Call/helper/inline-asm slot effect safety | No fact proves calls/helpers/inline asm are safe for slot `#21` over the interval. |
| Publication/move-bundle/parallel-copy non-clobber classification | Visible events are not classified as non-clobbering for slot `#21`. |

Rejected current evidence:

- The nearby `move from_value_id=16 to_value_id=17 destination_storage=stack_slot`
  remains rejected. It copies `%t22` storage into `%t23` storage with
  `authority=none`; it is not semantic materialization of `%t23 = ne i32 %t22,
  0`.
- Final stack homes, storage, offsets, object ids, raw BIR adjacency, value
  names, block names, testcase shape, and dump order remain insufficient.

Protected boundaries:

| Boundary row | Disposition |
| --- | --- |
| `%t22` select-result stack destination | Preserve as separate select-result/block-entry stack-destination owner. |
| `%t1` / `%t7` pointer/provenance rows | Preserve as separate pointer-value/provenance owner. |
| `%t2` / `%t8` unsupported-terminator rows | Preserve as separate branch-site relationship / terminator owner. |
| Downstream branch-stack-load authority | Remains unavailable until real `available` source-fact records exist and a later consumer owns the transition. |
| RV64 branch-load emission | Out of scope for idea 475. |

Artifacts:

- `build/agent_state/475_step3_source_fact_population_blocker/blocker.md`

## Suggested Next

Execute Step 4 from `plan.md`: Residual Disposition And Close Readiness. Record
that idea 475 should close or split because the first remaining owner is a
lower-level prepared producer for semantic instruction-result frame-slot
materialization/write records plus path/no-clobber interval facts.

## Watchouts

- Do not edit `src/backend/prealloc/publication_plans.*` or tests for this
  blocker route.
- Do not reopen the carrier/status surface from idea 474; the carrier can
  represent the facts, but the producer facts are absent.
- Do not mark `PreparedBranchStackLoadAuthority` available, consume these rows
  in RV64, or infer source facts from raw shape.
- Keep protected boundary rows out of the source-fact population route.

## Proof

Delegated proof:

```sh
git diff --check
```

Result: passed.
