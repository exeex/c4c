Status: Active
Source Idea Path: ideas/open/23_x86_prepared_edge_publication_remaining_home_coverage.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate the x86 Slice

# Current Packet

## Just Finished

Completed Step 4 validation for idea 23 after the Step 2/3 implementation was
committed. The code slice added shared lookup-backed
`RematerializableImmediate -> Register` edge-publication moves for x86 when the
source home publishes `immediate_i32`, rendering direct `mov reg, imm` output
through the existing shared `edge_publications` lookup path.

Focused proof before commit passed with the exact delegated command:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_prepared_decoded_home_storage|backend_x86_prepared_handoff_label_authority|backend_codegen_route_x86_64_.*|backend_prepared_lookup_helper|backend_prealloc_block_entry_publications)$' >> test_after.log 2>&1`

Result: passed with 79/79 selected tests. Supervisor ran the matching
regression guard against the focused `test_before.log` and it passed before the
slice was committed.

Broader backend validation after commit also passed with:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`

Result: passed with 162/162 backend tests. This backend run is broader
validation only, not a matching before/after regression comparison, because the
accepted `test_before.log` at the time was the focused 79-test scope.

## Suggested Next

Step 5 should make the RISC-V readiness handoff decision. The current x86
coverage now includes stack-source, register-source, and rematerializable
immediate-source edge-publication moves to register destinations through shared
lookup authority.

## Watchouts

- `PointerBasePlusOffset -> Register` and any source to `StackSlot`
  destination are intentionally still fail-closed.
- The implementation depends on shared `edge_publications`; it does not scan
  BIR edges or rebuild predecessor/successor facts locally.
- Do not claim RISC-V readiness unless Step 5 explicitly decides that the
  remaining fail-closed x86 homes are no longer blockers.
