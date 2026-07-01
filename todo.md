Status: Active
Source Idea Path: ideas/open/476_semantic_instruction_result_frame_slot_materialization_facts.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route Semantic Materialization Producer

# Current Packet

## Just Finished

Completed Step 3 for idea 476 by adding the smallest independent prepared
semantic materialization / interval record-status surface.

Implemented surface:

- Added `PreparedSemanticMaterializationIntervalInputs`,
  `PreparedSemanticMaterializationInterval`, event-kind and status vocab, plus
  `plan_prepared_semantic_materialization_interval` /
  `prepared_semantic_materialization_interval_available`.
- The surface accepts only explicit semantic result identity, explicit
  materialization/write event, matching target frame slot/object, path coverage,
  same-slot-write exclusion, and non-clobber classifications for calls/helpers,
  publications, move bundles, and parallel copies.
- It is independent from `PreparedFrameSlotSourceFact` population,
  branch-stack-load authority, prepared printers, and RV64 lowering.

Focused coverage:

| Case | Expected status |
| --- | --- |
| Synthetic explicit `%t23` semantic result written to matching slot `#21` with full interval/effect proof | `available` |
| Missing opcode / semantic result identity | `missing_semantic_result_identity` |
| Representative-style real `%t23` with no materialization/write event | `missing_materialization_event` |
| `%t22 -> %t23` storage-only source move | `materialization_value_mismatch` |
| Destination slot/object mismatch | `materialization_destination_mismatch` |
| Missing or non-covering path proof | `missing_path_validity` / `path_not_covering_consumer` |
| Same-slot writes unknown or found | `same_slot_write_unknown` / `same_slot_write_found` |
| Unknown or clobbering call/helper, publication, move-bundle, or parallel-copy effects | Corresponding effect-unknown or clobber status |
| Protected boundary row | `unsupported_boundary` |

Artifacts:

- `build/agent_state/476_step3_semantic_materialization_interval_surface/summary.md`
- `test_after.log`

## Suggested Next

Execute Step 4 from `plan.md`: Residual Disposition And Close Readiness.
Classify whether idea 476 is close-ready as an independent status surface or
whether a later source-fact population / real materialization event producer
should be split or activated.

## Watchouts

- The Step 3 surface proves explicit synthetic records only; it does not claim
  real `%t23` availability without real materialization/write and interval
  facts.
- `%t22 -> %t23` storage movement remains rejected as semantic
  materialization.
- `PreparedFrameSlotSourceFact` population, branch-stack-load authority, and
  RV64 lowering remain untouched.
- Keep select-result, pointer/provenance, and unsupported-terminator rows in
  separate owner families.

## Proof

Delegated proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed.
