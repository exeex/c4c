Status: Active
Source Idea Path: ideas/open/477_real_semantic_materialization_interval_fact_population.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route Real Semantic Fact Population

# Current Packet

## Just Finished

Completed Step 3 for idea 477 as a routed blocker. No implementation was
selected because the current prepared/prealloc surface does not expose a
durable semantic materialization/write event carrier for scalar
instruction-result frame-slot writes.

Blocker classification:

| Required fact | Current carrier state | Step 3 disposition |
| --- | --- | --- |
| Semantic identity for `%t23 = ne i32 %t22, 0` | Present through BIR instruction and prepared branch condition. | Usable input only. |
| Destination slot/object for `%t23` | Present through value home/storage and object `#21`. | Usable input only. |
| Semantic materialization/write event | No prepared event says the semantic compare result `%t23` was written/materialized into slot `#21`. | Missing required carrier; blocks real population. |
| `%t22 -> %t23` stack move | Present as `before_instruction authority=none`, `from_value_id=16 to_value_id=17`, destination stack slot. | Rejected storage movement; cannot be used as semantic materialization. |
| Path/no-clobber interval facts | No event-to-consumer path proof, same-slot write exclusion, or effect non-clobber facts exist. | Missing secondary facts; cannot be populated until event carrier exists. |
| Source-fact / branch-stack-load / RV64 consumers | Downstream of available semantic facts. | Untouched and remain blocked. |

Implementation/API blocker:

- The existing `PreparedSemanticMaterializationInterval` planner can accept
  explicit synthetic facts, but real collection has no authoritative producer
  event to feed it.
- Existing prepared facts only prove identity, destination home/storage, and
  move bundles. Treating those as a semantic write would require raw-shape
  inference or reuse of the rejected `%t22 -> %t23` storage-only move.
- Therefore Step 3 must route to a lower-level producer/API initiative that
  publishes semantic instruction-result frame-slot write/materialization events
  with event site, source result identity, destination slot/object, and
  authority before real interval population can proceed.

Artifacts:

- `build/agent_state/477_step3_real_semantic_fact_population/blocker.md`
- `build/agent_state/477_step3_real_semantic_fact_population/summary.md`
- `test_after.log`

## Suggested Next

Execute Step 4 from `plan.md`: Residual Disposition And Close Readiness.
Classify idea 477 as blocked/split-needed unless the plan owner chooses a
lower-level semantic write-event carrier producer as the next active owner.

## Watchouts

- Do not add real population by inferring from raw BIR adjacency, final stack
  homes, storage offsets, object ids, dump order, or testcase shape.
- Do not accept `%t22 -> %t23` `authority=none` storage movement as semantic
  compare-result materialization.
- Do not touch downstream `PreparedFrameSlotSourceFact`,
  `PreparedBranchStackLoadAuthority`, or RV64 lowering while the event carrier
  is missing.
- Keep select-result stack-destination, pointer/provenance, and
  unsupported-terminator boundaries separate.

## Proof

Delegated proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed.
