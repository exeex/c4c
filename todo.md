Status: Active
Source Idea Path: ideas/open/01_shared_call_plan_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Strengthen Shared Prepared Facts for the Chosen Family

# Current Packet

## Just Finished

Step 2 strengthened the shared prepared call-boundary effect endpoint for the
callee-saved preservation family.

`PreparedCallBoundaryEffectEndpoint` now carries the preservation storage
facts needed by the next AArch64 consumer packet:
`register_name`, `register_placement`, `occupied_register_names`, and
`spill_slot_placement`, in addition to the existing value, bank, width,
save-index, slot, offset, size, and alignment facts.

`make_preserved_storage_endpoint()` populates those fields from
`PreparedCallPreservedValue`, and
`backend_call_boundary_effect_plan_test` asserts they are present on the
storage endpoint for both `PreservationHomePopulation` and
`PreservationRepublication` effects.

## Suggested Next

Implement the AArch64 consumer packet for callee-saved preservation boundary
effects: route `lower_before_call_moves()` and `lower_after_call_moves()`
through `plan_prepared_call_boundary_effects()` preservation entries, pass the
selected prepared effect into the preservation emission helper, and stop using
direct `call_plan.preserved_values` iteration as the preservation effect
decision source.

## Watchouts

- Do not start AArch64 calls file consolidation before shared authority is
  proven.
- Do not hard-code AArch64-only facts into the shared planner.
- Do not weaken tests or add named-case matching as proof of progress.
- Preservation effect endpoints now carry storage placement facts, but they do
  not carry the AArch64 live-use gates. Keep the existing AArch64
  republication liveness decisions unless the prepared layer gains equivalent
  facts in a separate packet.

## Proof

`bash -lc '{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_call_boundary_effect_plan|backend_aarch64_call_boundary_owner|backend_codegen_route_aarch64_prepared_call_boundary_scalability)$"; } > test_after.log 2>&1'`

Proof passed: build completed and all three selected tests passed. Proof log:
`test_after.log`.
