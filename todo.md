# Current Packet

Status: Active
Source Idea Path: ideas/open/254_aarch64_globals_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Move Dispatch Lowering Body Behind Globals Owner

## Just Finished

Step 4 - Move Dispatch Lowering Body Behind Globals Owner completed as a
behavior-preserving globals redistribution packet.

Moved the address-materialization lowering body and diagnostic message/append
formatting from `dispatch.cpp` into
`src/backend/mir/aarch64/codegen/globals.cpp`, with the lowering helper
declared from `globals.hpp`. `dispatch.cpp` now includes `globals.hpp` and
remains the router, preserving visited-operation behavior and scalar-register
tracking after a returned address-materialization instruction.

## Suggested Next

Delegate the next globals redistribution packet only if the source plan has a
remaining globals-owned codegen body or declaration cleanup; otherwise send the
active plan back for lifecycle review instead of widening this shard.

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
- Do not pull in generic printer dispatch, `instruction.cpp/hpp`, tests, or
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
