Status: Active
Source Idea Path: ideas/open/673_post_664_full_suite_regression_probe.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Row 139 Direct-Global Local-Memory Publication Reload

# Current Packet

## Just Finished

Step 2 repaired row 139's RV64 object-emission direct-global local-memory
publication reload. The stale fail-close for live direct-global pointer
publication reloads was removed from `object_emission.cpp`, so the existing
prepared store-local publication emits the direct-global address into the frame
slot and the normal prepared local load path reloads it.

Focused evidence is under
`build/agent_state/673_step2_row139_direct_global_publication/`.

## Suggested Next

Supervisor should review and commit the row-139 slice if the diff is acceptable,
then ask the plan owner whether idea 673 can close or needs lifecycle follow-up.

## Watchouts

- Do not touch `review/reviewA.md`; it is a transient review artifact.
- Row 176 remains split to
  `ideas/open/674_rv64_object_terminator_lowering.md` and was not touched.

## Proof

Setup build:

`cmake --build build --target c4cll c4c-objdump backend_riscv_object_emission_test`

Executor proof:

`ctest --test-dir build -j --output-on-failure -R '^(backend_cli_riscv64_pointer_global_local_publication|backend_riscv_object_emission)$' > test_after.log 2>&1`

Result: passed. Row 139
`backend_cli_riscv64_pointer_global_local_publication` passed, and guard row
256 `backend_riscv_object_emission` passed. That proof log was copied to
`build/agent_state/673_step2_row139_direct_global_publication/focused_row139_row256_ctest.log`.

Supervisor comparable guard:

`ctest --test-dir build -j --output-on-failure -R '^(backend_cli_riscv64_pointer_global_local_publication|backend_obj_runtime_rv64_indirect_store_postincrement_callee_contract|backend_riscv_object_emission)$' > test_after.log 2>&1`

`test_before.log` was copied from
`build/agent_state/673_step1_regression_probe/focused_three_row_ctest.log`.
Regression guard result: passed (`passed=1 failed=2 total=3` before,
`passed=2 failed=1 total=3` after), resolving row 139 with no new failing
tests. Row 176 remains the only failing row in the comparable proof and belongs
to `ideas/open/674_rv64_object_terminator_lowering.md`.
