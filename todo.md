Status: Active
Source Idea Path: ideas/open/24_riscv_prepared_edge_publication_register_destination_consumer.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Broaden to Additional Ready Source Homes

# Current Packet

## Just Finished

Completed Step 4 by implementing `RematerializableImmediate -> Register`
prepared edge-publication consumption for the RISC-V register-destination
helper.

The helper still consumes authority only through
`prepare::find_unique_indexed_prepared_edge_publication(...)`. When the shared
publication is available, the move is a normal move publication, the destination
home is a register, and `source_home.immediate_i32` is present, RISC-V now emits
target-local immediate load syntax:

`li <destination-register>, <immediate>`

Existing `Register -> Register` behavior is preserved as `mv <dst>, <src>`.
The focused test now proves the positive immediate-source path, removed shared
lookup authority, malformed immediate homes, pointer-base sources, stack
sources, stack destinations, missing publication facts, missing lookup
authority, and non-move publications.

## Suggested Next

Proceed to the next supervisor-selected validation/review packet for idea 24,
or decide whether Step 4 has enough RISC-V register-destination source-home
coverage to hand off toward closure.

## Watchouts

- `PointerBasePlusOffset -> Register` and every source-to-`StackSlot`
  destination still fail closed and remain out of scope for this route unless
  lifecycle state changes.
- `StackSlot -> Register` remains ready in the shared prepared data model but
  deferred until RISC-V stack-slot load/address emission policy is handled as a
  separate packet.
- Do not claim pointer-base addressing, stack destinations, or stack-slot source
  loads from this packet; they remain unsupported/fail-closed.

## Proof

Proof command:

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_prepared_edge_publication|backend_codegen_route_riscv64_.*|backend_prepared_lookup_helper|backend_prealloc_block_entry_publications)$' >> test_after.log 2>&1`

Result: PASS. The selected subset passed 5/5 tests.

Supervisor ran the matching regression guard against the focused
`test_before.log` and focused `test_after.log` with non-decreasing pass count
allowed because this packet extended an existing CTest binary rather than
adding a new CTest name. Result: PASS with 5/5 before and 5/5 after, no new
failures.

Supervisor then ran broader backend validation:

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`

Result: PASS. Build succeeded and CTest reported 163/163 backend tests passing.
This backend run is broader validation only, not the matching before/after
regression comparison for the focused packet.
