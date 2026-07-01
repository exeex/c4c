Status: Active
Source Idea Path: ideas/open/487_bir_dynamic_local_array_proof_source_path_no_clobber_population.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route Real Proof Population

# Current Packet

## Just Finished

Completed Step 3 routing packet for idea 487.

Route decision:

| Route question | Step 3 answer |
| --- | --- |
| Is real proof-source/path/no-clobber population implemented now? | No. It remains blocked by missing dynamic local-array consumer-coordinate / prepared-exposure authority. |
| Exact blocker | Current dynamic local-array path records do not identify the GEP/access consumer block label and instruction index or expose a stable prepared lookup key shared with prepared control-flow facts. |
| Proposed prerequisite owner/title | `BIR Dynamic Local-Array Consumer Coordinate Prepared Exposure`. |
| Required prerequisite fields | Shared function identity, path result, source object, derivation result, dynamic index value, element type/size/count/offset/status, consumer block label, consumer instruction index, consumer operation role, stable prepared lookup key, and unsupported-boundary status. |
| Why not implement proof population now? | It would require raw-shape/name/proximity/dump-order matching between branch facts and dynamic local-array consumers, which is explicitly rejected. |
| Later idea 487 packet after prerequisite | Populate normalized branch/compare proof-source records, proof-to-consumer path/dominance coverage, and index no-clobber/same-value interval facts, then feed the idea 486 checker. |

Supporting artifacts:

- `build/agent_state/487_step1_real_proof_population_inputs/audit.md`
- `build/agent_state/487_step2_real_proof_population_contract/contract.md`
- `build/agent_state/487_step3_route_consumer_coordinate_prerequisite/route.md`

## Suggested Next

Execute Step 4 residual disposition for idea 487: decide whether this idea is
close-ready as a routed blocker, should remain active only after the prerequisite
carrier lands, or should be split by activating the proposed
`BIR Dynamic Local-Array Consumer Coordinate Prepared Exposure` owner.

## Watchouts

- Do not infer range proof from loop shape, variable names, testcase names,
  dump order, final homes, or RV64 target behavior.
- Do not mark dynamic rows available without real proof-source,
  path/dominance, and no-clobber facts accepted by the idea 486 checker.
- Do not change idea 486 checker/status vocabulary, idea 484 packaging, scalar
  local-load consumption, or RV64/MIR lowering in this population packet.
- Do not touch `review/`, canonical logs, baseline files, implementation files,
  or tests until an executor receives a bounded packet.
- Do not attempt branch proof-source matching by result names, loop shape, dump
  order, or implicit proximity between a branch and a dynamic GEP/access.
- Keep idea 486 checker/status vocabulary unchanged; the missing work is real
  producer evidence, not another synthetic checker case.
- Do not create implementation/test changes in idea 487 until the consumer
  coordinate/prepared-exposure carrier exists.
- Preserve idea 485 carrier, idea 484 packaging, scalar local-load consumption,
  and RV64/MIR lowering as separate boundaries.

## Proof

Step 3 validation:

```sh
git diff --check
```

Result: passed.
