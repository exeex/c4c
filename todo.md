Status: Active
Source Idea Path: ideas/open/253_aarch64_memory_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Move Memory Lowering And Address Helpers

# Current Packet

## Just Finished

Step 3 moved the existing F128 memory transport lowering route from
`src/backend/mir/aarch64/codegen/dispatch.cpp` into
`src/backend/mir/aarch64/codegen/memory.cpp` behind the memory-owned
`lower_f128_transport_instruction` helper declared in `memory.hpp`.

The F128 transport diagnostic formatter moved with the lowering route into
`memory.cpp`. `dispatch.cpp` now calls the memory-owned helper and keeps the
existing scalar-state integration append logic around the returned instruction.

## Suggested Next

Continue Step 3 with the next memory-owner extraction candidate if the
supervisor wants to keep moving memory/address helper ownership out of
`dispatch.cpp`; leave I128 pair, runtime-helper, and unrelated dispatch routes
in dispatch unless a separate packet says otherwise.

## Watchouts

The F128 transport route still emits the same machine instruction and
diagnostics; this packet only changed ownership. The dispatch integration point
deliberately remains the append/scalar-state block that records a returned
memory result register when present.

I128 transport, I128 pair operation lowering, F128 runtime-helper lowering, and
runtime-helper/non-memory transport behavior remain in `dispatch.cpp`. This
packet did not expand F128 semantics, invent carriers, or change unsupported
route boundaries.

## Proof

Ran exact delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

Result: green. `test_after.log` contains the combined build and backend CTest
output, ending with 139/139 backend tests passed.

Requested clang-tool checks confirmed the moved helper is now defined in
`memory.cpp` and still has the expected dispatch caller in
`dispatch_prepared_block`.
