Status: Active
Source Idea Path: ideas/open/477_real_semantic_materialization_interval_fact_population.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Real Semantic Fact Population Inputs

# Current Packet

## Just Finished

Completed Step 1 audit for idea 477 by classifying real prepared inputs for
`930930-1` `%t23 = ne i32 %t22, 0` into slot `#21`.

Real population audit:

| Required input | Current real evidence | Population classification |
| --- | --- | --- |
| Semantic instruction-result identity | `logic.end.14` contains `%t23 = bir.ne i32 %t22, 0`; branch condition records `condition=%t23 compare=ne i32 %t22, 0`. | Present and usable as identity input. |
| Consumer branch site | `bir.cond_br i32 %t23, block_5, block_6`; prepared branch condition names true/false labels. | Present and usable as consumer input. |
| Destination slot/object identity | `%t23` has `value_id=17`, home/storage `slot#21+stack156`, object `#21` type `i32 size=4 align=4`. | Present and usable as destination input. |
| Materialization/write event | No prepared record says the semantic compare result `%t23` was written/materialized into slot `#21`. | Missing first producer fact. |
| `%t22 -> %t23` storage-only move | `before_instruction authority=none block_index=11 instruction_index=3`, `move from_value_id=16 to_value_id=17 destination_storage=stack_slot`. | Rejected; storage movement is not semantic materialization. |
| Path / dominance / edge-scope proof | CFG and consumer are visible, but no event-to-consumer proof exists because no event exists. | Missing interval fact. |
| Same-slot write classification | No slot `#21` interval write classifier is present. | Missing interval fact. |
| Call/helper/inline-asm safety | Callsite data exists elsewhere, but no slot `#21` interval effect classifier exists. | Missing effect-safety fact. |
| Publication effects | Block-entry publications for `%t22` are visible and unsupported; no slot `#21` non-clobber classification exists. | Missing publication non-clobber fact; `%t22` publication is protected boundary. |
| Move-bundle effects | Move bundles are visible, including `%t22 -> %t23` with `authority=none`, but no slot-specific non-clobber interval classification exists. | Missing move-bundle non-clobber fact; storage-only move rejected. |
| Parallel-copy effects | Parallel copies into `%t22` are visible; no slot `#21` non-clobber classification exists. | Missing parallel-copy non-clobber fact; `%t22` select-result family remains out of scope. |

Protected boundaries:

| Boundary row | Evidence | Disposition |
| --- | --- | --- |
| `%t22` select-result stack destination | `select_chain root_is_select=yes`; block-entry publications to `%t22` are `unsupported_destination_storage`. | Out of scope for idea 477; separate select-result/block-entry stack-destination owner. |
| `%t1` / `%t7` pointer/provenance rows | Prior rows remain pointer/provenance boundaries. | Out of scope. |
| `%t2` / `%t8` unsupported-terminator rows | Branch-stack-load records remain `unsupported_terminator`. | Out of scope. |
| Source-fact population / branch-stack-load authority / RV64 | Real semantic materialization/interval records are unavailable. | Downstream blocked. |

Selected Step 2 target:

- Define the real population contract for using existing semantic identity and
  destination facts while requiring an explicit semantic materialization/write
  event and interval no-clobber facts.
- The likely first Step 3 packet is a real producer for semantic
  materialization/write events and interval classifiers, or a precise blocker
  if current prepared data has no durable event carrier.

Artifacts:

- `build/agent_state/477_step1_real_semantic_fact_population_audit/audit.md`
- `build/agent_state/477_step1_real_semantic_fact_population_audit/evidence_snippets.txt`

## Suggested Next

Execute Step 2 from `plan.md`: Define Real Population Contract. The contract
should accept the existing semantic identity and destination inputs but require
explicit materialization/write, path coverage, same-slot exclusion, and
non-clobber effect facts before any real `%t23` record can become available.

## Watchouts

- Do not classify `%t22 -> %t23` `authority=none` storage movement as semantic
  compare-result materialization.
- Do not resume `PreparedFrameSlotSourceFact` population, branch-stack-load
  authority, or RV64 lowering from identity/destination facts alone.
- Keep `%t22` select-result stack-destination, pointer/provenance, and
  unsupported-terminator boundaries separate.
- Step 2 should name an exact event/fact carrier requirement if real
  materialization/write population is not representable.

## Proof

Delegated proof:

```sh
git diff --check
```

Result: passed.
