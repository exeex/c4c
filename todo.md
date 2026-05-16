Status: Active
Source Idea Path: ideas/open/253_aarch64_memory_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Move Memory Lowering And Address Helpers

# Current Packet

## Just Finished

Step 3 moved the existing I128 memory transport lowering route from
`src/backend/mir/aarch64/codegen/dispatch.cpp` into
`src/backend/mir/aarch64/codegen/memory.cpp` behind the memory-owned
`lower_i128_transport_instruction` helper declared in `memory.hpp`.

The I128 transport diagnostic formatter moved with the lowering route into
`memory.cpp`. `dispatch.cpp` now calls the memory-owned helper and keeps the
existing instruction append/integration logic around the returned instruction.

## Suggested Next

Continue Step 3 with any remaining address-helper or memory-owner extraction
candidate the supervisor wants to delegate, or move to plan review if the
memory transport ownership split is sufficient for this step.

## Watchouts

The I128 memory transport route still emits the same machine instruction and
diagnostics; this packet only changed ownership. The dispatch integration point
deliberately remains the append block for the returned instruction.

I128 pair operation lowering, I128 runtime-helper handling, F128 runtime-helper
handling, and unrelated dispatch logic remain in `dispatch.cpp`. This packet did
not expand I128 semantics, invent carriers, or change unsupported route
boundaries.

## Proof

Ran exact delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

Result: green. `test_after.log` contains the combined build and backend CTest
output, ending with 139/139 backend tests passed.

Requested clang-tool checks confirmed `lower_i128_transport_instruction` is now
defined in `memory.cpp` and still has the expected dispatch caller in
`dispatch_prepared_block`.
