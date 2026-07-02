Status: Active
Source Idea Path: ideas/open/557_bir_local_memory_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Repair The Direct Local-Slot Producer/Consumer Boundary

# Current Packet

## Just Finished

Step 7 - Repair The Direct Local-Slot Producer/Consumer Boundary completed as
a verification/repair-complete packet.

Inspection found Step 6 already landed the direct local-slot producer/consumer
repair: direct non-pointer scalar local store/load paths publish neutral
LocalSlot `MemoryAddress` facts with local-slot provenance, requested range,
complete scalar extent, and scalar layout authority; pointer-valued direct
locals remain addressless to preserve pointer-carrier provenance. The prepared
route tests cover matching those facts against prepared frame-slot authority,
rejecting prepared address drift, and preserving the byval/source-identity
guardrails. Stack-layout classification treats self-contained scalar LocalSlot
facts as metadata rather than address-taking uses, while byte-storage overlays
and real addressed local uses still require home-slot/address-exposed behavior.

No additional implementation or test edit was needed for Step 7.

## Suggested Next

Proceed to Step 8 by proving the selected representative backend-object row
`src/20001026-1.c` with the supervisor-delegated RV64 command, then inspect
its `case.log` to confirm whether the store local-memory admission point moved
for producer-owned reasons.

## Watchouts

Reject target exclusions, testcase/helper-name shaped rules, expectation
rewrites, unsupported-marker changes, allowlist changes, and RV64/MIR inference.
Do not infer prepared/route compatibility from the RV64 row alone.

Remaining neighboring representative families are still intentionally left for
later packets until RV64 backend-object evidence is refreshed: load
`src/20000314-1.c`, GEP `src/20000717-4.c`, scalar/local-memory
`src/20000519-1.c`, and alloca `src/20050604-1.c`. The store representative
`src/20001026-1.c` has not yet been re-proved in this packet.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`

Result: passed, `345/345` backend tests. Proof log: `test_after.log`.
