Status: Active
Source Idea Path: ideas/open/673_post_664_full_suite_regression_probe.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce And Classify Regression Rows

# Current Packet

## Just Finished

Step 1 reproduced and classified the two rejected baseline-candidate rows.
Focused evidence is under
`build/agent_state/673_step1_regression_probe/`.

- Row 139 `backend_cli_riscv64_pointer_global_local_publication` fails during
  `--codegen obj` before object bytes are checked:
  `unsupported_local_memory_access: RV64 object route keeps live direct-global
  local pointer publication reloads fail-closed`. The prepared dump succeeds
  and shows available `store_local_publication` plus direct-global
  `address_materialization`, so the first owner is RV64 object emission for
  local-memory/direct-global pointer-publication reloads, not CLI dump
  publication or stale baseline state.
- Row 176
  `backend_obj_runtime_rv64_indirect_store_postincrement_callee_contract`
  fails during `--codegen obj` before clang link or QEMU runtime:
  `unsupported_terminator_fragment: BIR terminator requires unsupported RV64
  object lowering`. The prepared dump succeeds and shows a prepared fused
  pointer compare branch with available branch stack load authorities, so the
  first owner is RV64 object emission terminator lowering, not runtime behavior
  or stale baseline state.
- Guard row 256 `backend_riscv_object_emission` passed in the delegated
  three-row proof and in an individual guard proof.

## Suggested Next

Split before repair: keep row 139 in this active idea for a narrow
local-memory/direct-global pointer-publication object-emission packet, and
move row 176 to a separate idea for RV64 object terminator lowering unless the
supervisor prefers the opposite prioritization.

## Watchouts

- Do not touch `review/reviewA.md`; it is a transient review artifact.
- Do not reopen row 256 unless `backend_riscv_object_emission` regresses.
- Do not change expectations, unsupported markers, allowlists, timeout policy,
  runtime policy, or baseline accounting.
- Rows 139 and 176 share the broad RV64 object-emission phase, but their first
  failing contracts differ. Do not couple a direct-global local-memory
  publication repair with terminator lowering without new evidence.

## Proof

Setup build:

`cmake --build build --target c4cll c4c-objdump backend_riscv_object_emission_test`

Supervisor-selected proof:

`ctest --test-dir build -j --output-on-failure -R '^(backend_cli_riscv64_pointer_global_local_publication|backend_obj_runtime_rv64_indirect_store_postincrement_callee_contract|backend_riscv_object_emission)$' > test_after.log 2>&1`

Result: failed as expected for rows 139 and 176, with row 256 passing. The
canonical proof log is `test_after.log`, copied to
`build/agent_state/673_step1_regression_probe/focused_three_row_ctest.log`.

Additional diagnostics:

- `ctest --test-dir build -j --output-on-failure -R '^backend_cli_riscv64_pointer_global_local_publication$'`
- `ctest --test-dir build -j --output-on-failure -R '^backend_obj_runtime_rv64_indirect_store_postincrement_callee_contract$'`
- `ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'`
- `build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/backend/case/riscv64_pointer_global_local_publication.c`
- `build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/backend/case/riscv64_indirect_store_postincrement_callee_contract.c`
