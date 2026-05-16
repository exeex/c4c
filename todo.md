Status: Active
Source Idea Path: ideas/open/253_aarch64_memory_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Move Memory Spelling And Printer-Side Bodies

# Current Packet

## Just Finished

Step 4 moved the shared `MemoryOperand` address spelling helper from
`src/backend/mir/aarch64/codegen/machine_printer.cpp` into
`src/backend/mir/aarch64/codegen/memory.cpp` behind the memory-owned
`memory_address` declaration in `memory.hpp`.

`machine_printer.cpp` now includes `memory.hpp` and continues to use the helper
for inline-asm memory operands, spill/reload addresses, ordinary memory
instructions, I128 memory transport, and F128 memory transport without changing
the printer dispatch bodies or assembly spelling.

## Suggested Next

Continue Step 4 with the next printer-side memory body move the supervisor
wants to delegate, or route to review if the memory-owned spelling extraction
is sufficient for this step.

## Watchouts

The moved helper preserves the prior frame-slot and pointer-register
base+offset spelling. It now calls `abi::register_name(address.base_register->reg)`
directly because the printer-local `register_name(RegisterOperand)` wrapper did
not move with this packet.

Broad printer dispatch, non-memory printer bodies, I128/F128 transport printer
bodies, and atomic printer bodies remain in `machine_printer.cpp`.

## Proof

Ran exact delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

Result: green. `test_after.log` contains the combined build and backend CTest
output, ending with 139/139 backend tests passed.

Requested clang-tool checks confirmed `memory_address` is now defined in
`memory.cpp` and that `machine_printer.cpp` still has the expected callers:
inline-asm operand text, spill/reload printing, ordinary memory printing, I128
transport printing, and F128 transport printing.
