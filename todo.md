Status: Active
Source Idea Path: ideas/open/126_aarch64_memory_va_list_field_local_owner.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Choose the Local Owner Boundary

# Current Packet

## Just Finished

Completed Step 2 of `plan.md`: chose the smallest AArch64-local owner boundary
for the `va_list` field memory helper cluster without moving implementation
code.

Selected boundary:

- Keep the owner inside `src/backend/mir/aarch64/codegen/memory.cpp` as a
  grouped private/file-local surface for the next implementation packet.
- The owner should gather the existing helper cluster together and expose only
  local query/emission operations to `lower_memory_instruction`.
- Do not introduce a new translation unit or public header declaration in the
  Step 3 slice. The current helper dependencies are still tightly coupled to
  `memory.cpp` internals such as memory record builders, memory diagnostics,
  scalar sizing, register/address formatting, and inline machine-node
  construction.
- `memory.hpp` should not change for the Step 3 packet because the selected
  surface is consumed only by `lower_memory_instruction` in the same
  translation unit.
- Build metadata should not change for the Step 3 packet because no new file is
  selected yet.

Narrow caller shape for `lower_memory_instruction`:

- For local loads, ask the local owner for a prepared `va_list` field load
  record using `(context, instruction_index, const bir::LoadLocalInst&)`.
  If the owner returns no record, keep the existing ordinary
  `make_prepared_load_memory_instruction_record` fallback unchanged.
- For local stores, ask the local owner first for a cursor-update
  `module::MachineInstruction` using
  `(context, instruction_index, const bir::StoreLocalInst&)`. If it returns an
  instruction, return it from `lower_memory_instruction` as handled.
- If no cursor-update instruction is produced, ask the owner for a prepared
  `va_list` field store record using
  `(context, instruction_index, const bir::StoreLocalInst&)`. If the owner
  returns no record, keep the existing ordinary
  `make_prepared_store_memory_instruction_record` fallback unchanged.
- Global load/store paths should not call the owner.
- The owner must consume prepared variadic entry-plan layout/homes,
  value/storage homes, and AArch64-local register/assembler helpers; it must
  not recompute variadic layout, choose helper homes, or move shared
  prepared/BIR authority.

Rejected alternatives:

- Rejected moving the cluster to a new private `.cpp`/`.hpp` pair for Step 3:
  AST-backed callee checks show the cluster currently relies on many
  `memory.cpp`-local helpers, so a file split would either widen private helper
  declarations or force unrelated movement before ownership is clearer.
- Rejected adding declarations to `memory.hpp`: no external caller needs this
  route, and exporting it would widen the public AArch64 memory surface for a
  private `lower_memory_instruction` detail.
- Rejected sharing the route with `variadic.cpp`: that file has adjacent
  suffix parsing context, but this owner is specifically AArch64 memory
  lowering for prepared field loads, stores, and cursor-update emission.
- Rejected moving prepared fact selection into the owner: the source idea keeps
  prepared/BIR authority in shared prepared code, while this cleanup owns only
  target memory operand and instruction emission.

AST-backed checks used:

- `make_va_list_field_load_memory_instruction_record`,
  `make_va_list_field_store_memory_instruction_record`, and
  `make_va_list_field_cursor_update_machine_instruction` each have
  `lower_memory_instruction` as their only direct caller.
- The helper callees include local memory record builders, scalar-size helpers,
  value-home helpers, register/address formatting, scratch-register helpers,
  memory operand recording, and machine-instruction construction.

## Suggested Next

Execute Step 3 as a bounded implementation packet in
`src/backend/mir/aarch64/codegen/memory.cpp` plus `todo.md`: group the
`va_list` field helper cluster behind a private/file-local owner surface inside
`memory.cpp`, then reconnect only the existing local-load and local-store calls
in `lower_memory_instruction` through that surface. Do not edit `memory.hpp`,
build metadata, source idea, or tests unless the grouping causes unavoidable
include fallout.

## Watchouts

- Keep this as an AArch64-local ownership cleanup.
- Do not move or recompute shared prepared variadic facts.
- Do not weaken tests or rewrite expectations to claim progress.
- The Step 3 slice should preserve exact fallback behavior: owner miss means
  ordinary prepared local-load/local-store lowering still runs, and global
  memory instructions bypass the owner.
- If Step 3 unexpectedly requires declarations for helpers currently private to
  `memory.cpp`, keep the slice in `memory.cpp` and report the wider split as a
  later Step 4/5 decision instead of widening `memory.hpp`.
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

Delegated proof for this packet:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_(memory_operand_records|prepared_memory_operand_records|memory_operand_contract)|backend_prepare_frame_stack_call_contract|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff(_aarch64_publication)?|c_testsuite_aarch64_backend_src_00204_c)$' > test_after.log 2>&1
```

Result: build succeeded (`ninja: no work to do`) and all 8 focused tests passed.
Proof log: `test_after.log`.
