Status: Active
Source Idea Path: ideas/open/442_pointer_value_memory_provenance_publication.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Focused Coverage

# Current Packet

## Just Finished

Step 4 - Add Focused Coverage completed for the accepted Step 3 disposition.

Added BIR coverage in `backend_prepare_stack_layout_test.cpp` that:

- checks all three external-linkage same-module formal pointer loads remain
  non-target-consumable while `FormalPointerAuthorityKind` stays `Unknown`
- strengthens the pointer-value memory authority contract so unknown layout,
  `OpaqueCompatibility`, `UnknownCompatible` ranges, unknown object extent,
  bare pointer identities, missing pointer names, non-base-plus-offset access,
  out-of-bounds ranges, and negative pointer-delta ranges remain fail-closed

Added MIR coverage in `backend_riscv_object_emission_test.cpp` that rejects an
sret same-module call source selection carrying `source_pointer_byte_delta=-8`,
so RV64 object emission does not accept pointer arithmetic as target-side
inference.

## Suggested Next

Supervisor should review the Step 4 slice for acceptance and decide whether the
active plan is exhausted or needs a follow-up diagnostic/lifecycle packet.

## Watchouts

The external-linkage same-module formal fixture may carry producer-side
`OpaqueCompatibility` metadata today, but `prepared_pointer_value_memory_has_proven_authority`
still rejects it. The coverage intentionally proves target-consumability stays
false rather than changing producer publication in this packet.

## Proof

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Passed. `test_after.log` is the canonical proof log for this packet.
