Status: Active
Source Idea Path: ideas/open/488_bir_dynamic_local_array_consumer_coordinate_prepared_exposure.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route Prepared Exposure

# Current Packet

## Just Finished

Completed Step 3 repair for idea 488 by making the bounded
dynamic-local-array coordinate exposure explicitly represent a LIR
producer-site coordinate for the `address_derivation` role. The field/key/status
names no longer imply a prepared traversal or BIR `Block::insts` instruction
coordinate.

Implementation summary:

| Surface | Result |
| --- | --- |
| Coordinate carrier | `LocalArrayElementPathRecord` now carries `lir_producer_function_name`, `lir_producer_block_label`, `lir_producer_instruction_index`, `lir_producer_operation_role`, `lir_producer_lookup_key`, and `lir_producer_coordinate_status`. |
| Role vocabulary | Added explicit LIR producer-site `address_derivation` plus reserved fail-closed adjacent roles for load/store/unknown producers. |
| Status vocabulary | Added explicit LIR producer coordinate statuses for available, missing LIR coordinate/key pieces, duplicate/mismatch classes, unsupported role, protected boundary, raw-shape-only, and target/final-home-only cases. |
| Producer threading | Local-memory GEP lowering threads LIR block/instruction coordinates into local-array path publication. |
| Stable lookup key | Published with a `lir-producer:` prefix as `lir-producer:function:block:instruction:path_result:source_object:derivation_result:index...`. |
| Focused coverage | Local-array carrier tests assert LIR producer coordinate fields, `address_derivation`, `available`, and stable lookup keys for constant and dynamic element paths. A PHI-before-GEP fixture proves the index is the LIR producer-site index, not a prepared/BIR instruction index. |
| Boundaries preserved | No proof-source/path/no-clobber population, idea 486 checker vocabulary changes, idea 484 packaging, scalar local-load consumption, or RV64/MIR behavior changes. |

Supporting artifact:

- `build/agent_state/488_step3_consumer_coordinate_exposure/summary.md`

## Suggested Next

Execute Step 4 residual disposition for idea 488: classify the coordinate
exposure slice as complete or identify the next first owner for real
proof-source/path/no-clobber population after the prepared coordinate surface.

## Watchouts

- The new coordinate surface is address-derivation metadata only; load/store
  use-linking and proof-source/path/no-clobber population remain separate
  families.
- The coordinate is intentionally a LIR producer-site coordinate. Later
  prepared traversal/BIR instruction consumers need an explicit conversion or
  matching packet before treating it as a `bir_block->insts` coordinate.
- Dynamic local-array range proof status remains unchanged; dynamic rows still
  fail closed until explicit range/path/no-clobber facts are populated.
- Do not use this packet as authority for idea 484 packaging, scalar local-load
  consumption, or RV64/MIR lowering.
- Existing untracked `review/*.md` files were not touched.

## Proof

Step 3 validation:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed.

Proof log:

- `test_after.log`
