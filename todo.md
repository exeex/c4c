Status: Active
Source Idea Path: ideas/open/253_aarch64_memory_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Create Memory Owner And Move Operand Construction Routes

# Current Packet

## Just Finished

Step 2 moved the atomic-memory selected-node construction helpers from
`src/backend/mir/aarch64/codegen/instruction.cpp` into
`src/backend/mir/aarch64/codegen/memory.cpp`. The memory owner now holds atomic
operation error/kind naming, atomic opcode/status/side-effect helpers,
scratch-register selection for atomic loops, `make_atomic_memory_instruction`,
and `make_prepared_atomic_operation_instruction_record`.

`memory.hpp` now declares the atomic-memory helper surface alongside the
ordinary memory helper surface. `instruction.cpp` no longer defines the moved
atomic-memory helpers; `instruction.hpp` retains compatibility declarations for
existing broad include users.

## Suggested Next

Continue Step 2 with the next memory-family extraction candidate, likely the
remaining dispatch-side atomic-memory block route if the supervisor wants the
route selector body moved after this helper ownership slice.

## Watchouts

`memory.cpp` now owns local memory-family copies of the small effect and
scratch-register helpers needed by ordinary and atomic memory construction.
This preserves behavior without widening instruction-owned helper interfaces;
a future shared-helper cleanup can normalize duplication if desired.

Do not expand behavior while moving ownership: local pointer loads, global
loads, over-aligned alloca, dynamic stack, memcpy, and F128 memory-backed carrier
gaps remain unsupported route boundaries unless prepared carriers are added.
This packet did not invent support for those gaps or change atomic semantics; it
only moved the existing atomic-memory record/node construction bodies.

`dispatch.cpp` still calls `memory_error_message` for I128/F128 transport
memory-operand diagnostics. The message body is memory-owned, but those
transport routes remain in dispatch for now.

## Proof

Ran exact delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

Result: green. `test_after.log` contains the combined build and backend CTest
output, ending with 139/139 backend tests passed.

Requested clang-tool checks also confirmed
`make_prepared_atomic_operation_instruction_record` and
`make_atomic_memory_instruction` definitions are in `memory.cpp`.
