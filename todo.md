# Current Packet

Status: Active
Source Idea Path: ideas/open/254_aarch64_globals_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Create Globals Owner Shell

## Just Finished

Step 2 - Create Globals Owner Shell completed as a behavior-preserving owner
shell packet.

Created `src/backend/mir/aarch64/codegen/globals.hpp` and
`src/backend/mir/aarch64/codegen/globals.cpp` with the local AArch64 codegen
namespace/include style and no declarations or behavior moved yet.

Added `src/backend/mir/aarch64/codegen/globals.cpp` to the explicit
`C4C_BACKEND_SOURCES` AArch64 codegen source list in `src/backend/CMakeLists.txt`.

## Suggested Next

Delegate the next globals redistribution packet to move globals-family record
construction from `instruction.cpp` into `globals.cpp` behind minimal
`globals.hpp` declarations, while keeping generic instruction packaging in
`instruction.*` unless the compile boundary proves otherwise.

## Watchouts

- Keep this plan scoped to globals shard redistribution.
- Do not downgrade tests, mark supported globals paths unsupported, or rewrite
  expectations to claim progress.
- Preserve direct, GOT, label, string-constant, and TLS address materialization
  behavior through structured prepared facts and machine instruction records.
- The first real movement should not pull in generic instruction packaging,
  dispatch loop control, scalar-register tracking, or generic printer dispatch.
- `globals.cpp` now compiles as an empty owner shell; preserve the no-behavior
  baseline when moving the first globals-only helpers.
- Existing focused coverage for this family is concentrated in
  `backend_aarch64_prepared_memory_operand_records`,
  `backend_aarch64_machine_printer`, and `backend_aarch64_instruction_dispatch`.

## Proof

Proof passed and `test_after.log` was refreshed with the exact delegated proof
output.

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_(prepared_memory_operand_records|machine_printer|instruction_dispatch)$'
```
