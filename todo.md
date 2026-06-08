Status: Active
Source Idea Path: ideas/open/126_aarch64_memory_va_list_field_local_owner.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Reconnect `lower_memory_instruction` and Fallback

# Current Packet

## Just Finished

Completed Step 4 of `plan.md`: verified the current `lower_memory_instruction`
reconnect and fallback behavior after the private/file-local
`VaListFieldMemoryOwner` grouping.

Verification:

- Local loads ask `va_list_fields.load_record(...)` first and fall back to the
  ordinary prepared local-load record when the owner does not produce a record.
- Local stores ask `va_list_fields.cursor_update_instruction(...)` first, then
  `va_list_fields.store_record(...)`, and fall back to the ordinary prepared
  local-store record when neither special route applies.
- Cursor-update stores return the owner-produced machine instruction directly
  before ordinary prepared store lowering.
- Global loads and global stores bypass `VaListFieldMemoryOwner` and continue
  through their prepared global load/store overloads.
- AST-backed caller checks confirm `load_record`, `store_record`, and
  `cursor_update_instruction` are called only by `lower_memory_instruction`.
- No reconnect or fallback bug was found, so no implementation files were
  edited.

## Suggested Next

Execute Step 5 as focused proof and close-decision preparation: use the current
focused proof plus supervisor-selected acceptance criteria to decide whether
this AArch64-local ownership cleanup is ready to close or needs broader backend
validation before commit.

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

Result: build succeeded (`ninja: no work to do`) and all 8 focused tests passed.
Proof log: `test_after.log`.

Supervisor broader acceptance retained from the Step 3 code slice:

```sh
ctest --test-dir build -j --output-on-failure -R '^backend_'
```

Result: all 179 backend tests passed.
