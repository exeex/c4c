Status: Active
Source Idea Path: ideas/open/253_aarch64_memory_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Create Memory Owner And Move Operand Construction Routes

# Current Packet

## Just Finished

Step 2 moved the remaining dispatch-side atomic-memory block lowering route
from `src/backend/mir/aarch64/codegen/dispatch.cpp` into
`src/backend/mir/aarch64/codegen/memory.cpp` behind the memory-owned
`lower_atomic_memory_operations_for_block` helper.

`dispatch.cpp` now only selects the route, counts the returned atomic machine
instructions, and appends them to the block. The atomic prepared-operation
diagnostic formatter moved with the lowering route into `memory.cpp`, and
`memory.hpp` declares the new helper surface.

## Suggested Next

Continue Step 2 with the next memory-family extraction candidate, likely one of
the remaining dispatch-owned memory transport routes if the supervisor wants to
keep reducing dispatch-side memory ownership.

## Watchouts

The atomic route still emits the same machine instructions and diagnostics; the
dispatch integration point deliberately remains an append loop over the returned
instructions.

Do not expand behavior while moving ownership: local pointer loads, global
loads, over-aligned alloca, dynamic stack, memcpy, and F128 memory-backed carrier
gaps remain unsupported route boundaries unless prepared carriers are added.
This packet did not invent support for those gaps or change atomic semantics; it
only moved the existing atomic-memory block-lowering body.

`dispatch.cpp` still calls `memory_error_message` for I128/F128 transport
memory-operand diagnostics. The message body is memory-owned, but those
transport routes remain in dispatch for now.

## Proof

Ran exact delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

Result: green. `test_after.log` contains the combined build and backend CTest
output, ending with 139/139 backend tests passed.

Requested clang-tool checks confirmed the removed dispatch helper had one
caller in `dispatch_prepared_block`; the new helper is declared in `memory.hpp`,
defined in `memory.cpp`, and called from dispatch.
