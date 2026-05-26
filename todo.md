Status: Active
Source Idea Path: ideas/open/23_x86_prepared_edge_publication_remaining_home_coverage.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Remaining x86 Edge Homes

# Current Packet

## Just Finished

Completed Step 1 inventory for idea 23: the x86 prepared edge-publication
consumer already accepts shared lookup-backed `StackSlot -> Register` moves
(`mov reg, DWORD PTR [rsp + offset]`) and `Register -> Register` moves through
`consume_edge_publication_move_intent` / `append_edge_publication_move_instruction`.
That covers the prior ideas 21/22 source-home families while preserving shared
publication authority, prepared value ids, destination storage kind, block-entry
parallel-copy step ordering, and fail-closed behavior when shared lookups or a
publication are missing.

Remaining nearby home combinations:

- `RematerializableImmediate -> Register`: unsupported today as
  `UnsupportedSourceHome`, but it is the safest next source-home family because
  the destination contract stays register-only and x86 can render a direct
  immediate-to-register `mov`.
- `PointerBasePlusOffset -> Register`: keep unsupported for now; it needs typed
  width/base rendering rather than a generic edge-publication shortcut.
- `None` or missing source home -> any destination: keep fail-closed as
  `UnsupportedSourceHome` or missing authority.
- Any source -> `StackSlot` destination: keep unsupported for now. Register or
  immediate sources can become a later memory-destination family, but
  stack-to-stack would require temporary-register policy and should not be
  folded into the immediate-source packet.
- Any source -> `RematerializableImmediate`, `PointerBasePlusOffset`, `None`, or
  missing destination home: keep fail-closed as unsupported/missing destination
  authority.

Chosen next home family: `RematerializableImmediate -> Register`.

Why in scope: it extends the existing x86 prepared edge-publication source-home
renderer without changing lookup authority, destination storage policy, control
flow ownership, or implementation-file boundaries beyond the prepared consumer
helper. It is a semantic home-family expansion, not a testcase-specific route.

## Suggested Next

Implement `RematerializableImmediate -> Register` edge-publication support.
Owned files for that packet should be:
`src/backend/mir/x86/prepared/dispatch.cpp`,
`tests/backend/bir/backend_x86_prepared_decoded_home_storage_test.cpp`, and, if
the supervisor wants end-to-end compare-join coverage for the selected family,
`tests/backend/bir/backend_x86_handoff_boundary_joined_branch_test.cpp`.

Recommended proof command:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_prepared_decoded_home_storage|backend_x86_prepared_handoff_label_authority|backend_codegen_route_x86_64_.*|backend_prepared_lookup_helper|backend_prealloc_block_entry_publications)$' >> test_after.log 2>&1`

## Watchouts

- Do not add x86-local edge-copy facts or rediscovery.
- Do not weaken supported-path expectations or mark a targeted path
  unsupported as capability progress.
- Keep unsupported homes explicit and fail-closed.
- Do not widen the next packet into stack-slot destinations; direct
  memory-destination publication and stack-to-stack publication need separate
  policy.
- Do not treat `PointerBasePlusOffset` as an immediate-source variant; it needs
  base/register/addressing authority before x86 can render it safely.

## Proof

No build required by the delegated inventory-only packet; no root-level logs
were created.
