# Current Packet

Status: Active
Source Idea Path: ideas/open/254_aarch64_globals_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Move Globals Printer Spelling

## Just Finished

Step 5 - Move Globals Printer Spelling completed as a behavior-preserving
globals redistribution packet.

Moved direct, GOT, label, string-constant, and TLS address-materialization
printer spelling from `machine_printer.cpp` into
`src/backend/mir/aarch64/codegen/globals.cpp`, with the globals-owned printer
helper declared from `globals.hpp`. `machine_printer.cpp` now keeps the generic
payload dispatch and calls the globals-owned helper for
`AddressMaterializationRecord` payloads.

## Suggested Next

Send the active plan back for lifecycle review if Step 5 exhausts the current
globals shard runbook; otherwise delegate only the next explicitly remaining
globals-owned redistribution or declaration-cleanup packet.

## Watchouts

- Keep this plan scoped to globals shard redistribution.
- Do not downgrade tests, mark supported globals paths unsupported, or rewrite
  expectations to claim progress.
- Preserve direct, GOT, label, string-constant, and TLS address materialization
  behavior through structured prepared facts and machine instruction records.
- `instruction.hpp` still carries compatibility declarations for existing
  include sites; `globals.hpp` now owns the globals-facing declarations.
- The moved construction code keeps local storage/register conversion helpers
  in `globals.cpp` to avoid touching memory, dispatch, or generic instruction
  packaging in this packet.
- Generic instruction packaging remains in `instruction.cpp`; dispatch loop
  control and scalar-register tracking remain in `dispatch.cpp`.
- Machine-printer generic payload dispatch remains in `machine_printer.cpp`;
  globals-specific address-materialization spelling now lives behind
  `print_address_materialization_instruction`.
- Do not pull in non-globals printer bodies, `instruction.cpp/hpp`, tests, or
  expectation rewrites without a separate supervisor packet.
- Existing focused coverage for this family is concentrated in
  `backend_aarch64_prepared_memory_operand_records`,
  `backend_aarch64_machine_printer`, and `backend_aarch64_instruction_dispatch`.

## Proof

Proof passed and `test_after.log` was refreshed with the exact delegated proof
output.

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_(prepared_memory_operand_records|machine_printer|instruction_dispatch)$'
```
