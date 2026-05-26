Status: Active
Source Idea Path: ideas/open/23_x86_prepared_edge_publication_remaining_home_coverage.md
Source Plan Path: plan.md
Current Step ID: 2/3
Current Step Title: Implement shared-lookup consumption and focused coverage
你該做code review了
你該做test baseline review了

# Current Packet

## Just Finished

Completed Step 2 and Step 3 for idea 23: x86 prepared
`consume_edge_publication_move_intent` now accepts shared lookup-backed
`RematerializableImmediate -> Register` edge-publication moves when the source
home publishes `immediate_i32`, rendering direct `mov reg, imm` output through
the existing shared `edge_publications` lookup path.

Added focused decoded-home-storage coverage proving the positive immediate
source shape preserves shared publication authority, prepared value ids, source
home kind, destination register intent, and appended x86 text. Added negative
coverage proving immediate lowering emits nothing without shared lookup
authority, malformed immediate homes remain fail-closed, pointer-base source
homes remain unsupported, and stack-slot destinations remain unsupported.

## Suggested Next

Step 4 validation recommendation: supervisor should run the milestone broader
validation it wants for idea 23, with particular attention to compare-join
handoff paths that call
`append_edge_publication_move_instruction` from prepared control-flow emission.

## Watchouts

- This packet did not touch the end-to-end joined-branch fixture; the delegated
  subset still covered the x86 route tests selected by the supervisor.
- `PointerBasePlusOffset -> Register` and any source to `StackSlot`
  destination are intentionally still fail-closed.
- The implementation depends on shared `edge_publications`; it does not scan
  BIR edges or rebuild predecessor/successor facts locally.

## Proof

Ran exactly:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_prepared_decoded_home_storage|backend_x86_prepared_handoff_label_authority|backend_codegen_route_x86_64_.*|backend_prepared_lookup_helper|backend_prealloc_block_entry_publications)$' >> test_after.log 2>&1`

Result: passed; `test_after.log` contains the build and selected CTest output
with 79/79 tests passing.
