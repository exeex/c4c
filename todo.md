Status: Active
Source Idea Path: ideas/open/488_bir_dynamic_local_array_consumer_coordinate_prepared_exposure.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Consumer Coordinate Exposure Contract

# Current Packet

## Just Finished

Completed Step 2 contract definition for idea 488.

Contract:

| Contract field | Step 2 decision |
| --- | --- |
| Preferred implementation shape | Producer-site coordinate capture: thread BIR block label and instruction index into local-memory/GEP lowering and publish coordinate-bearing dynamic local-array path records. |
| Required coordinate fields | Function identity, path result, source object, derivation result, dynamic index value, element type, element size, element count, byte offset, source path status, consumer block label, consumer instruction index, consumer operation role, and stable prepared lookup key. |
| Supported first operation role | `address_derivation`, meaning the local-array GEP/path-producing instruction itself. |
| Deferred/rejected roles | Load consumers, store consumers, unknown consumers, and target/final-home-only consumers stay fail-closed until separate use-linking packets exist. |
| Prepared lookup key | Function + block + instruction + path result + source object + derivation result + dynamic index. |
| Fallback unique-match policy | A post-lowering collector is allowed only if it proves exactly one matching BIR instruction/result and one path record, with matching function/source/derivation/index where available; otherwise it fails closed. |
| Fail-closed statuses | Missing block/instruction/key, duplicate coordinate candidates, duplicate path records, mismatched function/block/result/source/derivation/index, unsupported role, protected boundaries, raw-shape-only, and target/final-home-only inference. |
| Step 3 readiness | A bounded implementation packet exists for metadata/exposure only; no proof-source/path/no-clobber population or downstream consumers. |

Step 3 should implement the smallest metadata/exposure surface for the
`address_derivation` role or route the exact API blocker if producer-site
coordinate threading conflicts with the current lowerer signatures.

Supporting artifacts:

- `build/agent_state/488_step1_consumer_coordinate_exposure_inputs/audit.md`
- `build/agent_state/488_step2_consumer_coordinate_exposure_contract/contract.md`

## Suggested Next

Execute Step 3: implement or route the bounded dynamic local-array
consumer-coordinate/prepared-exposure metadata packet for the
`address_derivation` role, with focused positive and fail-closed coverage.

## Watchouts

- Do not populate proof-source, path/dominance, or no-clobber facts in this
  coordinate exposure packet.
- Do not infer coordinates from dump order, testcase names, value names, branch
  proximity, loop shape, final homes, or RV64 target behavior.
- Do not mark dynamic range proofs available, change idea 486 checker
  vocabulary, package idea 484 records, consume scalar local loads, or change
  RV64/MIR lowering.
- Do not touch `review/`, canonical logs, baseline files, implementation files,
  or tests until an executor receives a bounded packet.
- Prefer explicit coordinate capture at the local-array path producer over
  post-hoc matching; if post-hoc matching is selected, require unique exact
  instruction/result evidence and fail closed otherwise.
- Do not treat prepared traversal coordinates alone as proof that a traversal
  event is the local-array consumer; the path record must be explicitly linked.
- Keep load/store use-linking, proof-source/path/no-clobber population, idea
  484 packaging, scalar local-load consumption, and RV64/MIR lowering out of
  Step 3.

## Proof

Step 2 validation:

```sh
git diff --check
```

Result: passed.
