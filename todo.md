Status: Active
Source Idea Path: ideas/open/666_rv64_callee_saved_gpr_runtime.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Repair The Selected Callee-Saved/Live-Value Rule

# Current Packet

## Just Finished

Step 3 repaired the selected RV64 object-route callee-saved/live-value
consumer ordering boundary for rows 183, 184, and 219.

Changed files:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_emit_context.hpp`
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.hpp`
- `todo.md`
- `test_after.log`

The object-route call fragment now records the published GPR call-result
destination and suppresses after-call preservation republication that would
write a stale callee-saved value back over that same register. This matches the
existing text-route rule without naming rows or hard-coding registers.

The RV64 prepared scalar consumer context now carries the current BIR block, so
the freshness check can distinguish values defined after the latest same-block
call from truly live-through caller-saved homes. A post-call local reload or
call result can therefore be consumed from its fresh direct home, while
genuinely stale caller-saved homes still fall back to the prepared
prior-preservation path.

Rows 183 and 184 preserve the CLI/codegen static-storage object-data checks,
and rows 183, 184, and 219 pass under the same general ordering repair.

## Suggested Next

Have the supervisor review and commit this Step 3 slice, then decide whether
the active plan needs a broader RV64 prepared-object regression guard or can
advance to the next planned callee-saved/runtime boundary.

## Watchouts

- This packet intentionally did not touch test expectations, unsupported
  markers, allowlists, timeout/runtime policy, baseline accounting, or adjacent
  backend families.
- The repair relies on BIR block context only where the object route already
  has it; unindexed scalar helper callers retain the previous conservative
  behavior.
- The prepared call plan for row 219 still publishes a prior-preservation fact
  for `%t3`, but the object route now consumes the later `%t3` load from its
  fresh `t0` home before using that preservation carrier.

## Proof

Proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_cli_riscv64_prepared_object_data_static_local_storage_obj|backend_cli_riscv64_prepared_object_data_static_local_initialized_storage_obj|backend_codegen_route_riscv64_prepared_object_data_static_local_storage|backend_codegen_route_riscv64_prepared_object_data_static_local_initialized_storage|backend_obj_runtime_rv64_prepared_object_data_static_local_storage|backend_obj_runtime_rv64_prepared_object_data_static_local_initialized_storage|backend_obj_runtime_rv64_callee_saved_gpr_live_across_call)$' > test_after.log 2>&1`

Result: passed, 7/7 tests. Proof log: `test_after.log`.

Supervisor acceptance:

- Focused regression guard compared matching seven-row logs and passed:
  before 4/7, after 7/7, with rows 183, 184, and 219 resolved and no new
  focused failures.
- Broader patched backend smoke ran
  `ctest --test-dir build -j --output-on-failure -R '^backend_'` and reported
  359/368 passing. The remaining failing backend rows were 92, 103, 109, 150,
  154, 172, 256, 284, and 322; target rows 183, 184, and 219 were not in the
  failing set.
