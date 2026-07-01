Status: Active
Source Idea Path: ideas/open/489_bir_dynamic_local_array_proof_population_from_lir_coordinates.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define LIR Coordinate Proof Population Contract

# Current Packet

## Just Finished

Completed Step 2 contract definition for idea 489.

Contract result:

| Contract area | Step 2 decision |
| --- | --- |
| Accepted representative | Scalar dynamic local-array `address_derivation` path with one dynamic index, `lir_producer_coordinate_status=available`, and a stable `lir-producer:` key. |
| Binding coordinate | Use `lir_producer_function_name`, `lir_producer_block_label`, `lir_producer_instruction_index`, `lir_producer_operation_role`, and `lir_producer_lookup_key`; never treat the LIR instruction index as a prepared traversal/BIR instruction index. |
| Proof source | Prepared `FusedCompare` branch-condition facts only, with structured operand match to the dynamic index. |
| Bounds | Lower bound normalized to `0` inclusive via supported lower predicate; upper bound normalized to `element_count` exclusive via supported upper predicate. |
| Path/dominance | Requires explicit proof-edge/path coverage and dominance or guard validity from proof source to the `lir_producer_*` site. |
| No-clobber | Requires explicit same-value/no-clobber interval facts for the dynamic index through redefinition, phi/alias, call/helper, inline-asm, publication/move, and parallel-copy effects. |
| Checker mapping | Maps `lir_producer_*` fields into idea 486 `LocalArrayIndexRangeProofInputs` consumer fields as LIR producer-site coordinates. |
| Step 3 decision rule | Implement only if targeted inspection finds authoritative path/no-clobber certificate producers; otherwise route to `dynamic local-array LIR producer path/no-clobber certificate`. |

Supporting artifact:

- `build/agent_state/489_step2_lir_coordinate_proof_population_contract/contract.md`

## Suggested Next

Execute Step 3: inspect for authoritative proof-source/path coverage and
same-value/no-clobber interval producers keyed to `lir_producer_*`. Implement a
bounded producer only if both exist; otherwise record the exact lower-owner
blocker without changing code.

## Watchouts

- Do not infer proof facts from loop shape, branch proximity, value names,
  testcase names, dump order, final homes, or target behavior.
- Do not treat `lir_producer_instruction_index` as a prepared traversal or BIR
  `Block::insts` instruction index.
- Do not mark real dynamic rows available without explicit proof-source,
  path/dominance, and no-clobber facts accepted by the idea 486 checker.
- Keep idea 486 checker vocabulary, idea 488 coordinate exposure, idea 484
  packaging, scalar local-load consumption, and RV64/MIR lowering out of scope.
- Existing untracked `review/*.md` files were not touched.

## Proof

Step 2 validation:

```sh
git diff --check
```

Result: passed.
