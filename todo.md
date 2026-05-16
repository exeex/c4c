Status: Active
Source Idea Path: ideas/open/253_aarch64_memory_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Create Memory Owner And Move Operand Construction Routes

# Current Packet

## Just Finished

Step 2 moved the dispatch-side ordinary memory lowering path into
`src/backend/mir/aarch64/codegen/memory.cpp` behind the memory-owned
`lower_memory_instruction` helper declared in `memory.hpp`. The moved body now
owns the prepared addressing lookup, ordinary load/store prepared-record
selection, memory-specific diagnostic append path, `make_memory_instruction`
node creation, and BIR machine-instruction wrapping for ordinary memory.

`memory_error_message` also moved from `dispatch.cpp` into the memory owner.
`dispatch.cpp` now includes `memory.hpp` and remains the route selector: it
tries F128 transport first, then routes unhandled memory-shaped BIR
instructions to the memory-owned ordinary helper while preserving the existing
scalar-state update for emitted memory loads.

## Suggested Next

Continue Step 2 by selecting the next clean memory-owned dispatch surface, with
atomic-memory block lowering as the likely candidate if its diagnostics and
record construction can move without crossing into printer, traversal, or
unsupported route expansion.

## Watchouts

`memory.cpp` currently has local copies of small register/effect helpers needed
to avoid widening broad instruction interfaces during this extraction. A future
shared-helper cleanup can normalize that duplication, but it was not required
for this packet.

Do not expand behavior while moving ownership: local pointer loads, global
loads, over-aligned alloca, dynamic stack, memcpy, and F128 memory-backed carrier
gaps remain unsupported route boundaries unless prepared carriers are added.
This packet did not invent support for those gaps; it only moved the existing
ordinary load/local-store/global-store lowering path.

`dispatch.cpp` still calls `memory_error_message` for I128/F128 transport
memory-operand diagnostics. The message body is memory-owned, but those
transport routes remain in dispatch for now.

## Proof

Ran exact delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

Result: green. `test_after.log` contains the combined build and backend CTest
output, ending with 139/139 backend tests passed.

Requested clang-tool checks also confirmed `lower_memory_instruction` and
`memory_error_message` definitions are in `memory.cpp`, and the ordinary memory
route caller is `dispatch_prepared_block` in `dispatch.cpp`.
