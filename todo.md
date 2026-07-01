Status: Active
Source Idea Path: ideas/open/475_prepared_frame_slot_source_fact_population.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Completed Step 4 residual disposition for idea 475 after the Step 3
source-fact population blocker.

Decision: idea 475 is close-ready as a routed/blocked population attempt. It
should not remain active for another implementation packet until a lower-level
prepared producer exists for semantic instruction-result frame-slot
materialization/write records plus path/no-clobber interval facts.

Residual disposition:

| Row/family | Current state | First remaining owner |
| --- | --- | --- |
| `f.logic.end.14` condition `%t23`, slot `#21` | Target identity is known, but no real source fact can be populated. The existing carrier can represent the fact, but no prepared event writes/materializes `%t23 = ne i32 %t22, 0` into slot `#21`. | Lower-level semantic instruction-result frame-slot materialization/write producer. |
| Path and interval facts for `%t23` | No dominance/path, same-slot exclusion, call/helper/inline-asm safety, or publication/move/parallel-copy non-clobber facts are available for slot `#21`. | Lower-level path/no-clobber interval producer. |
| `%t22 -> %t23` stack move | Rejected as evidence; `authority=none`, source value `#16`, destination value `#17`, not semantic compare-result materialization. | No owner in idea 475; future producer must create semantic event instead. |
| `%t22` select-result stack destination | Protected boundary. | Separate select-result/block-entry stack-destination owner. |
| `%t1` / `%t7` pointer/provenance rows | Protected boundary. | Separate pointer-value/provenance owner. |
| `%t2` / `%t8` unsupported-terminator rows | Protected boundary. | Separate branch-site relationship / terminator owner. |
| Downstream branch-stack-load authority and RV64 | Still blocked. | Later consumers only after real available source facts exist. |

Lifecycle recommendation: plan-owner should close or split idea 475. The
durable follow-up should be a new lower-level prepared producer idea for:

- semantic scalar instruction-result identity;
- explicit frame-slot materialization/write events;
- path/dominance or edge-scope proof to the consumer;
- same-slot write exclusion;
- call/helper/inline-asm slot effect safety;
- publication/move-bundle/parallel-copy non-clobber classification.

Artifacts:

- `build/agent_state/475_step4_source_fact_population_residual/disposition.md`

## Suggested Next

Hand to plan-owner for lifecycle closure/split. If work continues, activate a
new lower-level producer idea before returning to source-fact population,
branch-stack-load authority, or RV64 consumption.

## Watchouts

- Do not implement source-fact population from current evidence; it would
  require raw-shape inference.
- Do not reuse the `%t22 -> %t23` stack move as materialization evidence.
- Do not mark `PreparedBranchStackLoadAuthority` available or edit RV64
  lowering.
- Keep select-result stack-destination, pointer/provenance, and
  unsupported-terminator rows in their own owners.

## Proof

Delegated proof:

```sh
git diff --check
```

Result: passed.
