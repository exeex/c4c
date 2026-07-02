Status: Active
Source Idea Path: ideas/open/557_bir_local_memory_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Pin Direct Local-Slot Publication And Route Compatibility

# Current Packet

## Just Finished

Step 6 - Pin Direct Local-Slot Publication And Route Compatibility completed.
The slice adds focused coverage for direct same-slot scalar local store/load
publication, prepared/MIR route consumption of those LocalSlot address facts,
and byval/source-identity guardrails from the blocked attempt.

Implementation was required because the notes harness does not keep expected
failures. Direct scalar local-slot store/load paths now publish neutral
LocalSlot `MemoryAddress` provenance, requested range, complete scalar extent,
and scalar layout authority. Pointer-valued direct local stores/loads remain
addressless to preserve pointer-carrier provenance. Stack-layout classification
now treats self-contained scalar LocalSlot facts as metadata, not address-taking
uses, while byte-storage overlays and real addressed local uses still require
home-slot/address-exposed behavior.

## Suggested Next

Proceed to Step 7 with a narrow representative producer/admission packet for
`src/20001026-1.c`, using the new direct scalar LocalSlot facts as the semantic
contract rather than matching the testcase shape.

## Watchouts

Reject target exclusions, testcase/helper-name shaped rules, expectation
rewrites, unsupported-marker changes, allowlist changes, and RV64/MIR inference.
Do not infer prepared/route compatibility from the RV64 row alone.

During this packet, broad scalar LocalSlot address publication initially marked
ordinary locals address-exposed and disturbed RV64/byval route expectations.
The accepted implementation is intentionally limited to non-pointer direct
scalar locals and keeps self-contained scalar LocalSlot facts from forcing
address-taken stack objects.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`

Result: passed, `345/345` backend tests. Proof log: `test_after.log`.
