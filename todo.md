# Current Packet

Status: Active
Source Idea Path: ideas/open/811_lir_native_vector_authority_carrier_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish structured vector identities and facts

## Just Finished

Step 2 published opt-in `LirNativeVectorAuthority` carriers on insert,
extract, and shuffle schemas. The two scalar-splat seams now use native result
operands and publish result/vector/element IDs when available, lane/element
shapes, structured zero indices, and ordered zero-mask lanes; the vector
`IndexExpr` seam publishes its native result/vector/index facts. Function
ownership validation rejects incoherent owner/result/use IDs and incompatible
shape/index/mask mirrors without adding vector-row semantics.

## Suggested Next

Step 3: add focused valid/malformed carrier coverage for ownership and mirror
coherence only; do not add insert/extract/shuffle row semantics.

## Watchouts

No display-text recovery or synthetic IDs for `poison`/`zeroinitializer`.
Keep aggregate, CFG/PHI, pointers, Raw-BIR, target, MIR, emission, tests, and
all 754 row behavior outside this packet.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log` passed; `test_after.log` is the canonical proof log.
