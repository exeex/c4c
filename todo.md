Status: Active
Source Idea Path: ideas/open/126_aarch64_memory_va_list_field_local_owner.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Reconnect `lower_memory_instruction` and Fallback

# Current Packet

## Just Finished

Completed Step 3 of `plan.md`: grouped the AArch64 `va_list` field memory
helper cluster behind a private/file-local `VaListFieldMemoryOwner` surface in
`src/backend/mir/aarch64/codegen/memory.cpp` while preserving behavior.

Implementation:

- Added `VaListFieldMemoryOwner` as the local owner for prepared `va_list`
  field memory lowering in `memory.cpp`.
- Kept the former parsing, field-address lookup, memory operand construction,
  scalar-size check, cursor-update producer detection, load record, store
  record, and cursor-update machine-instruction emission together behind that
  owner.
- Exposed only three owner operations to `lower_memory_instruction`:
  `load_record`, `store_record`, and `cursor_update_instruction`.
- Reconnected local loads through `va_list_fields.load_record(...)` before the
  ordinary prepared local-load fallback.
- Reconnected local stores through
  `va_list_fields.cursor_update_instruction(...)` first, then
  `va_list_fields.store_record(...)`, before the ordinary prepared local-store
  fallback.
- Left global load/store paths behavior-equivalent and bypassing the owner.
- Did not edit `memory.hpp`, build metadata, tests, `plan.md`, or the source
  idea.

AST-backed caller checks used before the edit:

- The old load, store, and cursor-update helper entry points each had
  `lower_memory_instruction` as their only direct caller.

## Suggested Next

Execute Step 4 as a verification/status packet: confirm `lower_memory_instruction`
fallback behavior is equivalent after the owner grouping, then either record
Step 4 complete or report any remaining reconnect issue before Step 5 closure
validation.

## Watchouts

- Coverage weakness: the delegated subset proves the route mostly through
  integration and prepared-dump expectations; there is no narrow unit test that
  directly constructs a `BlockLoweringContext` and asserts
  `VaListFieldMemoryOwner` load/store/cursor-update selection and fallback
  behavior by owner method.
- This remains an AArch64-local ownership cleanup. Shared prepared variadic
  facts are still consumed, not recomputed or moved.

## Proof

Delegated proof for this packet:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_(memory_operand_records|prepared_memory_operand_records|memory_operand_contract)|backend_prepare_frame_stack_call_contract|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff(_aarch64_publication)?|c_testsuite_aarch64_backend_src_00204_c)$' > test_after.log 2>&1
```

Result: build succeeded, including `memory.cpp`, and all 8 focused tests passed.
Proof log: `test_after.log`.

Supervisor broader acceptance:

```sh
ctest --test-dir build -j --output-on-failure -R '^backend_'
```

Result: all 179 backend tests passed.
