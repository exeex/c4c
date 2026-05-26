Status: Active
Source Idea Path: ideas/open/22_x86_prepared_edge_publication_coverage_broadening.md
Source Plan Path: plan.md
Current Step ID: 2/3
Current Step Title: Broaden x86 Prepared Consumption + Add Focused Authority Tests
你該做test baseline review了

# Current Packet

## Just Finished

Completed plan Step 2, "Broaden x86 Prepared Consumption", and Step 3, "Add
Focused Authority Tests".

`x86::prepared::consume_edge_publication_move_intent` now accepts shared
edge-publication moves from `PreparedValueHomeKind::Register` source homes to
`PreparedValueHomeKind::Register` destination homes through the existing
`ConsumedPlans::shared_function_lookups()->edge_publications` lookup path. The
existing stack-slot source support remains intact, and missing shared lookups,
missing publications, unsupported publications, unsupported source homes, and
unsupported destination homes remain explicit statuses.

Focused tests now cover a direct register-source to register-destination move
intent, missing shared authority for that shape, and the prepared x86 module
route emitting register-source shared edge-publication moves while rejecting a
drifted shared-publication destination.

## Suggested Next

Proceed to plan Step 4 validation: supervisor should run the selected broader
regression guard or acceptance validation for this x86 prepared
edge-publication broadening slice, then decide whether the active runbook can
move toward lifecycle review or closure.

## Watchouts

- This packet intentionally did not add memory-destination support; stack-slot
  destination homes remain unsupported.
- The x86 consumer still does not scan BIR edges or reconstruct
  predecessor/successor facts locally; shared `edge_publications` remains the
  authority.
- Do not implement RISC-V consumer work in this plan.

## Proof

Passed:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_prepared_decoded_home_storage|backend_x86_prepared_handoff_label_authority|backend_codegen_route_x86_64_.*|backend_prepared_lookup_helper|backend_prealloc_block_entry_publications)$' >> test_after.log 2>&1`

Proof log: `test_after.log`.
