Status: Active
Source Idea Path: ideas/open/485_bir_local_array_address_derivation_index_range_authority_carrier.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route Carrier Producer

# Current Packet

## Just Finished

Completed Step 3 for idea 485 by implementing the bounded BIR local-array
carrier producer surface selected by Step 2:

- Added durable BIR carrier records for local array source objects, address
  derivations, and element paths on `bir::Function`.
- Populated source-object facts from static local array `alloca` lowering.
- Populated derivation and layout/path facts from explicit local array/view GEP
  paths.
- Preserved dynamic-index representatives as
  `missing_index_range_proof` rather than inferring loop/range authority.
- Kept aggregate/member GEPs fail-closed with no local-array carrier authority.
- Did not add scalar local-load consumption, idea 484 packaging, prepared
  consumers, or RV64/MIR lowering.

Focused coverage was added in `backend_lir_to_bir_notes_test` for:

| Case | Expected carrier result |
| --- | --- |
| Static `[4 x i32]` local array plus constant GEP | source-object, derivation, and element-path/layout records are `available` |
| Static `[4 x i32]` local array plus dynamic GEP | source-object/derivation/path shape exists, path status is `missing_index_range_proof` |
| Local aggregate/member GEP | no local-array carrier authority is published |

Artifact:

- `build/agent_state/485_step3_local_array_carrier_producer/summary.md`

## Suggested Next

Complete Step 4 residual disposition for idea 485: classify the carrier surface
as implemented for source-object/derivation/layout records, keep dynamic index
availability blocked on an explicit index range proof and path/dominance
producer, and decide whether the next owner is that lower-level proof carrier.

## Watchouts

- The Step 3 carrier records are producer metadata only; downstream scalar
  local-load consumption and RV64/MIR lowering remain out of scope.
- Dynamic representatives such as the `pr38048-1.c` `a#L1[i#L3][0]` shape
  still lack range/path proof and must remain unavailable until a producer
  publishes those facts.
- Aggregate/member, global, integer-pointer round-trip, variadic/runtime,
  F128, complex/vector, volatile/atomic, bootstrap, raw-shape-only, and
  target-only/final-home-only cases remain fail-closed.
- The new records are not printed yet; focused tests inspect lowered BIR
  directly.

## Proof

Delegated proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed; `test_after.log` contains the build and `^backend_` CTest
output.
