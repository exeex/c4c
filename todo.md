Status: Active
Source Idea Path: ideas/open/299_rv64_runtime_pointer_param_member_index_foundation.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Run acceptance validation and document boundaries

# Current Packet

## Just Finished

Steps 1 through 5 registered and proved
`backend_rv64_runtime_local_dynamic_member_array`. The rv64 prepared emitter now
materializes caller local aggregate address arguments from prepared
`local_frame_address_materialization` call-argument metadata, materializes the
callee pointer/member-array address from prepared pointer base plus scalar byte
offset, and lowers I32 `load_local` instructions whose prepared memory access
has `base=pointer_value`.

Unsupported pointer-memory forms still fail closed when metadata, register
homes, width, alignment, address space, or offset encoding do not match the
bounded contract.

## Suggested Next

The code slice is ready for supervisor acceptance and commit. After commit,
call the plan owner to decide whether the active runbook/source idea should
close or stay active for follow-up boundary work.

## Watchouts

- The narrow qemu proof now passes and generated `get_at` emits the pointer
  add, pointer-base I32 loads at offsets 0/4/8, select chain, and return.
- Keep the route limited to a direct scalar helper call that receives a pointer
  to a local aggregate and indexes a simple member array through prepared
  metadata.
- Do not accept globals, indirect calls, varargs, stack-passed arguments,
  aggregate ABI, dynamic stack support, or broad pointer provenance in this
  plan.
- Use prepared value homes, memory accesses, call plans, and address
  materialization metadata; do not match helper names, filenames, or exact
  source shapes.
- Step 3 also satisfies the Step 4 narrow runtime observable for this case:
  `backend_rv64_runtime_local_dynamic_member_array` exits with expected code
  `11` under qemu.
- The RISC-V focused acceptance command preserves one baseline failure:
  `backend_riscv_prepared_edge_publication` prints
  `RISC-V prepared module should emit a register edge move`. A backend-enabled
  committed-baseline comparison in `/tmp/c4c-compare` at
  `0c3b1bc62496752c96032bcc9fb175b3dd791475` shows the same failure while the
  two `backend_codegen_route_riscv64_*` tests pass.
- Keep review artifacts under `review/` untouched unless a supervisor-provided
  lifecycle task explicitly requires reading or rewriting around one.

## Proof

Ran exactly:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_rv64_runtime_local_dynamic_member_array$') > test_after.log 2>&1`

Result: build succeeded, CTest registered and ran
`backend_rv64_runtime_local_dynamic_member_array`, and the test passed under
qemu with expected status `11`. Proof log: `test_after.log`.

Acceptance validation:

- `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_rv64_runtime') > test_after.log 2>&1`: passed, 20/20 rv64 runtime tests green with the new case included.
- `ctest --test-dir build -j --output-on-failure -R 'backend_riscv|backend_codegen_route_riscv64' >> test_after.log 2>&1`: preserved baseline result, 2/3 passed and `backend_riscv_prepared_edge_publication` failed with the same assertion on the committed baseline.
