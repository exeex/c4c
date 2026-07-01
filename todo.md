Status: Active
Source Idea Path: ideas/open/489_bir_dynamic_local_array_proof_population_from_lir_coordinates.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit LIR Coordinate Proof Population Inputs

# Current Packet

## Just Finished

Completed Step 1 audit for idea 489.

Audit result:

| Input family | Step 1 classification |
| --- | --- |
| Dynamic local-array path records | Sufficient path payload exists on `LocalArrayElementPathRecord`, including dynamic index and layout, but dynamic rows remain `missing_index_range_proof`. |
| LIR producer coordinate | Idea 488's `lir_producer_*` fields and `lir-producer:` key are sufficient as an address-derivation binding surface. They must not be treated as prepared traversal/BIR instruction coordinates. |
| Prepared branch/compare facts | Candidate proof-source facts exist through prepared branch conditions and fused compare fields, but normalized lower/upper bound population still needs a contract. |
| Control-flow/path/dominance | Prepared control-flow labels and dominance precedents exist, but no durable proof-to-`lir_producer_*` path coverage certificate is currently exposed. |
| No-clobber/same-value interval | The idea 486 checker has statuses for these facts, but no current durable interval record proves the dynamic index is unchanged up to the LIR producer site. |
| Step 2 readiness | Step 2 can define a bounded proof-population contract around the `lir_producer_*` surface, while preserving that real availability remains blocked unless path/dominance and no-clobber certificates are explicit. |
| First missing lower owner | Durable dynamic local-array LIR producer path/no-clobber certificate surface: proof-edge coverage, dominance/guard validity, and same-value/no-clobber interval facts for the dynamic index. |

Supporting artifact:

- `build/agent_state/489_step1_lir_coordinate_proof_population_audit/audit.md`

## Suggested Next

Execute Step 2: define the proof-population contract from LIR producer
coordinates, including accepted branch/compare proof-source shapes, normalized
bound requirements, path/dominance requirements, no-clobber requirements, and
fail-closed routing if the missing certificate owner is still absent.

## Watchouts

- Do not infer proof facts from loop shape, branch proximity, value names,
  testcase names, dump order, final homes, or target behavior.
- Do not treat `lir_producer_instruction_index` as a prepared traversal or BIR
  `Block::insts` instruction index.
- Do not mark real dynamic rows available without explicit proof-source,
  path/dominance, and no-clobber facts accepted by the idea 486 checker.
- Do not change idea 486 checker vocabulary, idea 488 coordinate exposure, idea
  484 packaging, scalar local-load consumption, or RV64/MIR lowering.
- Existing untracked `review/*.md` files were not touched.

## Proof

Step 1 validation:

```sh
git diff --check
```

Result: passed.
