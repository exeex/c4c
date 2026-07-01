Status: Active
Source Idea Path: ideas/open/487_bir_dynamic_local_array_proof_source_path_no_clobber_population.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Real Proof Population Inputs

# Current Packet

## Just Finished

Completed Step 1 audit for idea 487.

Audit result:

| Input bucket | Existing facts | First missing fact | Step 1 classification |
| --- | --- | --- | --- |
| Dynamic local-array path | `LocalArrayElementPathRecord` carries result/source object/derivation/dynamic index/layout and `missing_index_range_proof`. | Consumer block label and instruction index, plus prepared exposure keyed to control-flow coordinates. | Lower-level carrier blocker. |
| Branch/compare proof source | `PreparedBranchCondition` carries function/block, fused compare predicate/type/lhs/rhs, and true/false labels. | Bound-normalized proof-source record linked to the same dynamic index and consumer interval. | Candidate input only. |
| Path/dominance | Prepared CFG blocks and dominance helpers exist. | Proof-to-consumer path certificate covering the dynamic local-array consumer. | Missing path authority. |
| No-clobber/same-value | Idea 486 checker has explicit no-clobber/redefinition/call/helper/publication/move status fields. | Real interval facts proving the dynamic index is unchanged and unclobbered between proof and consumer. | Missing no-clobber authority. |
| Boundaries | Idea 485 carrier, idea 484 packaging, scalar local-load consumption, and RV64 lowering are separate owners. | None for this packet. | Preserved fail-closed boundaries. |

No real proof-source/path/no-clobber population implementation is selected from
current facts. The first exact blocker is a durable dynamic local-array
consumer-coordinate / prepared-exposure carrier: current local-array path
records do not identify the GEP/load consumer block and instruction coordinate,
so prepared branch proof facts cannot be matched to a consumer interval without
raw-shape inference.

Supporting artifact:

- `build/agent_state/487_step1_real_proof_population_inputs/audit.md`

## Suggested Next

Execute Step 2: define the real proof population contract or route it to the
lower-level dynamic local-array consumer-coordinate / prepared-exposure carrier
required before branch proof-source, path/dominance, and index no-clobber
records can be populated.

## Watchouts

- Do not infer range proof from loop shape, variable names, testcase names,
  dump order, final homes, or RV64 target behavior.
- Do not mark dynamic rows available without real proof-source,
  path/dominance, and no-clobber facts accepted by the idea 486 checker.
- Do not change idea 486 checker/status vocabulary, idea 484 packaging, scalar
  local-load consumption, or RV64/MIR lowering in this population packet.
- Do not touch `review/`, canonical logs, baseline files, implementation files,
  or tests until an executor receives a bounded packet.

## Proof

Step 1 validation:

```sh
git diff --check
```

Result: passed.
