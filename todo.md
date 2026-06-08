Status: Active
Source Idea Path: ideas/open/126_aarch64_memory_va_list_field_local_owner.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Characterize the Current Route

# Current Packet

## Just Finished

Completed Step 1 of `plan.md`: characterized the current AArch64 `va_list`
field memory route before implementation changes.

Exact helper cluster in `src/backend/mir/aarch64/codegen/memory.cpp`:

- `parse_va_list_field_suffix`
- `VaListFieldAddress`
- `find_va_list_field_address`
- `make_va_list_field_memory_operand`
- `make_va_list_field_load_memory_instruction_record`
- `make_va_list_field_store_memory_instruction_record`
- `va_list_field_cursor_update_producer`
- `make_va_list_field_cursor_update_machine_instruction`

Current callers and route shape:

- `lower_memory_instruction` is the only consumer of the special route.
- Local loads first call `make_va_list_field_load_memory_instruction_record`;
  failure falls back to ordinary prepared local-load memory lowering.
- Local stores first call `make_va_list_field_cursor_update_machine_instruction`;
  if that does not match, they call `make_va_list_field_store_memory_instruction_record`;
  failure then falls back to ordinary prepared local-store memory lowering.
- Global loads and stores do not enter this special route.
- AST-backed lookup confirmed `find_va_list_field_address` has only three
  direct callers: the load record helper, the store record helper, and the
  cursor-update machine-instruction helper.

Prepared facts consumed:

- `find_va_list_field_address` reads `context.function.prepared`,
  `control_flow`, and `storage_plan`; looks up the prepared variadic entry plan
  for the current function; scans `helper_operand_homes` for the `VaStart`
  `destination_va_list`; parses slot names shaped like `<va_list-name>.<offset>`;
  matches offsets against `entry_plan->va_list_layout.fields`; validates the
  destination home against the storage plan; and converts the prepared register
  into an AArch64 X-register storage-plan operand.
- The load route additionally requires zero byte offset, scalar result size
  matching the prepared field size, a named result value, value-home lookup for
  the loaded result, and then reuses `make_load_memory_instruction_record`.
- The store route additionally requires zero byte offset, stored value scalar
  size matching the prepared field size, optional named stored-value identity,
  value-home lookup for that stored value, and then reuses
  `make_store_memory_instruction_record`.
- The cursor-update route additionally requires the immediately preceding BIR
  shape `load ptr slot; add ptr immediate; store ptr same slot`, non-negative
  stride, a scratch GP register distinct from the cursor/base register, and
  emits an inline assembler machine node with a memory operand record.
- No helper in this cluster creates or selects shared variadic facts; it only
  consumes prepared entry-plan layout/homes, storage/value-home facts, and
  AArch64-local register/assembler helpers.

Focused coverage recorded:

- `backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy` covers
  AArch64 variadic aggregate overflow byte-copy route assembly generation.
- `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff` covers generic
  prepared BIR handoff for the stdarg c-testsuite case.
- `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`
  covers AArch64 prepared publication facts for the same stdarg case, including
  va_arg add/store progression snippets and call-boundary facts.
- `c_testsuite_aarch64_backend_src_00204_c` covers the AArch64 backend
  c-testsuite stdarg case end to end.
- `backend_aarch64_memory_operand_records`,
  `backend_aarch64_prepared_memory_operand_records`, and
  `backend_aarch64_memory_operand_contract` cover ordinary memory record
  construction, metadata preservation, fail-closed behavior, and fallback
  contract surfaces.
- `backend_prepare_frame_stack_call_contract` is adjacent prepared frame/stack
  call-boundary coverage included in the delegated proof subset.

## Suggested Next

Execute Step 2: choose the smallest local owner boundary. Suggested packet:
decide whether the cluster stays as a grouped private namespace surface inside
`memory.cpp` or moves to a new private AArch64 memory helper file/header, then
record the chosen caller shape without implementation movement. Prefer an API
where `lower_memory_instruction` asks a local owner for load/store/cursor-update
handling and falls back unchanged when the owner returns no record.

## Watchouts

- Keep this as an AArch64-local ownership cleanup.
- Do not move or recompute shared prepared variadic facts.
- Do not weaken tests or rewrite expectations to claim progress.
- Coverage weakness: the delegated subset proves the route mostly through
  integration and prepared-dump expectations; there is no narrow unit test that
  directly constructs a `BlockLoweringContext` and asserts
  `make_va_list_field_load_memory_instruction_record`,
  `make_va_list_field_store_memory_instruction_record`, or cursor-update
  selection/fallback behavior by helper name.
- `src/backend/mir/aarch64/codegen/variadic.cpp` has a separate
  `parse_va_list_field_suffix` for variadic helper emission; Step 2 should
  treat that as adjacent context, not as part of this memory-owner extraction
  unless include fallout makes it unavoidable.

## Proof

Ran the delegated proof exactly:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_(memory_operand_records|prepared_memory_operand_records|memory_operand_contract)|backend_prepare_frame_stack_call_contract|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff(_aarch64_publication)?|c_testsuite_aarch64_backend_src_00204_c)$' > test_after.log 2>&1
```

Result: build succeeded (`ninja: no work to do`) and all 8 focused tests passed.
Proof log: `test_after.log`.
