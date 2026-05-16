Status: Active
Source Idea Path: ideas/open/253_aarch64_memory_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Create Memory Owner And Move Operand Construction Routes

# Current Packet

## Just Finished

Step 2 created `src/backend/mir/aarch64/codegen/memory.hpp` and
`src/backend/mir/aarch64/codegen/memory.cpp`, added `memory.cpp` to
`src/backend/CMakeLists.txt`, and moved the ordinary prepared memory operand
plus load/store selected-node construction bodies out of `instruction.cpp`.

Moved symbols/bodies include `memory_base_kind_name`,
`memory_operand_support_kind_name`, `memory_instruction_kind_name`,
`prepared_memory_operand_record_error_name`, `make_memory_operand`,
`make_memory_instruction`, the prepared memory operand validators and
`make_memory_record_from_prepared_access`, local/global
`make_prepared_memory_operand_record` overloads, frame-slot load instruction
record construction, and local/global store instruction record construction.
`instruction.hpp` still carries the aggregate public declarations so existing
callers and tests do not need a shared-header redesign, while `memory.hpp`
provides the new memory-owner declaration surface for codegen owners.

## Suggested Next

Continue Step 2 by moving the next memory-owned route out of broad files:
prefer the dispatch-side ordinary memory lowering/error-message path into a
memory-owned helper while leaving `dispatch.cpp` as the route selector.

## Watchouts

`memory.cpp` currently has local copies of small register/effect helpers needed
to avoid widening broad instruction interfaces during this extraction. A future
shared-helper cleanup can normalize that duplication, but it was not required
for this packet.

Do not expand behavior while moving ownership: local pointer loads, global
loads, over-aligned alloca, dynamic stack, memcpy, and F128 memory-backed carrier
gaps remain unsupported route boundaries unless prepared carriers are added.

## Proof

Ran exact delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

Result: green. `test_after.log` contains the combined build and backend CTest
output, ending with 139/139 backend tests passed.
