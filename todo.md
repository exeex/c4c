# Current Packet

Status: Active
Source Idea Path: ideas/open/254_aarch64_globals_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Delete Markdown Shard And Run Acceptance Proof

## Just Finished

Step 6 - Delete Markdown Shard And Run Acceptance Proof completed as the final
globals shard reconciliation packet.

Reconciled `src/backend/mir/aarch64/codegen/globals.md` against the active
globals implementation and deleted the obsolete markdown shard. Its durable
notes are represented by globals-owned preparation, lowering, and printer
helpers in `globals.cpp`/`globals.hpp`: direct globals, GOT globals, labels,
string constants, and TLS address materialization all flow through structured
prepared facts into `AddressMaterializationRecord` and globals-owned printer
spelling.

## Suggested Next

Send the active plan back for lifecycle review. The globals markdown shard has
been deleted after reconciliation, so further work should come from plan-owner
close/deactivate/split judgment instead of another executor packet on this
shard.

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
- The legacy `globals.md` scratch convention that always materialized through
  `x0` is explicitly obsolete; active code uses the prepared result home and
  allocation-backed `result_register`.
- The legacy `needs_got_aarch64(name)` note is represented by structured
  prepared `address_materialization_policy` checks for direct versus GOT
  materialization, not by string fallback.
- Do not reintroduce markdown mirrors, non-globals printer bodies,
  `instruction.cpp/hpp`, tests, or expectation rewrites without a separate
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
