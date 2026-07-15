# Current Packet

Status: Active
Source Idea Path: ideas/open/811_lir_native_vector_authority_carrier_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Verify carrier coherence and focused coverage

## Just Finished

Step 3 added direct LIR-verifier coverage for a valid insert/extract/shuffle
carrier set and malformed owner, result/use identity, vector shape/display
mirror, index value/type, and shuffle mask-lane/mask-mirror cases. Shuffle now
also requires its second vector shape whenever it carries a second vector use;
this is carrier coherence only and does not validate vector operation semantics.

## Suggested Next

Step 4: obtain the supervisor-selected regression guard and record the carrier
handoff for the return to 754 Step 9.

## Watchouts

The new tests call `verify_module` directly and do not assert lane selection,
element compatibility, shuffle input compatibility, poison behavior, or row
admission. Keep aggregate, CFG/PHI, pointers, Raw-BIR, target, MIR, emission,
and all 754 row behavior outside this packet.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log` passed; `test_after.log` is the canonical proof log.
