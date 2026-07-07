# Current Packet

Status: Active
Source Idea Path: ideas/open/582_rv64_va_start_stack_backed_destination.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add Focused Helper Coverage

## Just Finished

Step 2 from `plan.md` is complete.

Added focused RV64 object-emission coverage for the semantic `va_start` helper
shape where:

- `destination_va_list` is a prepared stack-slot home.
- `destination_va_list_address` is also stack-backed, in a distinct prepared
  stack slot.
- The current precise diagnostic remains
  `unsupported_variadic_helper_lowering: RV64 va_start helper requires destination va_list address in a prepared GPR home`.

The fixture is backend-local and does not depend on `src/va-arg-21.c`, source
text, function/block coordinates, or representative value names.

## Suggested Next

Execute Step 3 from `plan.md`: repair RV64 `va_start` destination-address
materialization for supported stack-backed `destination_va_list_address` homes,
or replace the current broad GPR-only rejection with a narrower fail-closed
destination-address materialization diagnostic.

## Watchouts

- Preserve the semantic helper shape; do not match `src/va-arg-21.c` by
  filename, function, block, value name, or source text.
- The focused negative test now asserts distinct stack slots for the va_list
  object and the stack-backed destination address before checking the current
  diagnostic.
- Keep unsupported non-stack or aliasing helper forms fail-closed.
- Preserve unrelated open ideas, including untracked
  `ideas/open/583_rv64_pointer_arithmetic_result_publication.md`.

## Proof

Proof log: `test_after.log`.

Command:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'; } > test_after.log 2>&1`

Result: passed, `backend_riscv_object_emission` 1/1.
