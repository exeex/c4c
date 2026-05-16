# Current Packet

Status: Active
Source Idea Path: ideas/open/254_aarch64_globals_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Move Address-Materialization Record Construction

## Just Finished

Step 3 - Move Address-Materialization Record Construction completed as a
behavior-preserving globals redistribution packet.

Moved globals-family address materialization kind selection, prepared-fact
identity validation, address-record construction, and prepared materialization
record wrappers from `instruction.cpp` into
`src/backend/mir/aarch64/codegen/globals.cpp`, with owner declarations in
`globals.hpp`. Generic instruction packaging, including
`make_address_materialization_instruction`, remains in `instruction.cpp`.

## Suggested Next

Delegate the next globals redistribution packet to move globals-owned lowering
or dispatch-adjacent glue only if the source plan identifies it as part of the
globals shard; otherwise send the active plan back for review before widening
beyond record construction.

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
- Do not pull in generic instruction packaging, dispatch loop control,
  scalar-register tracking, or generic printer dispatch without a separate
  supervisor packet.
- Existing focused coverage for this family is concentrated in
  `backend_aarch64_prepared_memory_operand_records`,
  `backend_aarch64_machine_printer`, and `backend_aarch64_instruction_dispatch`.

## Proof

Proof passed and `test_after.log` was refreshed with the exact delegated proof
output.

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_(prepared_memory_operand_records|machine_printer|instruction_dispatch)$'
```
