Status: Active
Source Idea Path: ideas/open/499_semantic_gep_local_memory_admission_producer.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route The First Producer Packet

# Current Packet

## Just Finished

Step 3 implemented the first production BIR semantic GEP local-memory producer
packet. `LocalArraySemanticGepRecord` records are populated only from matching
available `LocalArrayLocalAddressProvenanceRecord` authority for local-object
`AddressDerivation` paths, with fail-closed status propagation for non-local or
non-available lower authority. Summary:
`build/agent_state/499_step3_gep_local_memory_producer/summary.md`.

## Suggested Next

Record the residual disposition for idea 499: decide whether the focused
local-object GEP producer is sufficient to close this idea or whether a
follow-up producer idea should be activated for global/static GEPs or the
non-local boundary families.

## Watchouts

- This is a BIR semantic producer packet, not RV64/MIR lowering.
- Do not infer GEP authority from raw shape, names, final homes, lowered target
  behavior, or route-local slots.
- The producer consumes only matching production
  `LocalArrayLocalAddressProvenanceRecord` authority. Missing, duplicate, or
  non-available lower records remain fail-closed.
- Step 1 found only 3 direct local-object candidate rows. The 6 global/static
  object GEP rows are separated as later-owner boundaries, and the other 53 rows
  remain fail-closed or prerequisite-owned until explicit producer authority
  exists.
- Keep store local-memory, direct-call, scalar/local-memory, runtime,
  move-bundle, F128, stack-frame, and scalar-load work out of this packet.
- If a lower producer prerequisite is missing, route lifecycle to that
  prerequisite instead of implementing target inference.

## Proof

Step 3 implementation proof:

```sh
cmake --build build -j2 > test_after.log 2>&1 && ctest --test-dir build -j2 --output-on-failure -R '^backend_' >> test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
