Status: Active
Source Idea Path: ideas/open/666_rv64_callee_saved_gpr_runtime.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Refresh Callee-Saved And Live-Value Evidence

# Current Packet

## Just Finished

Lifecycle switched from retired static-storage idea 663 to active
callee-saved/live-value idea 666. The retired 663 evidence routed rows 183 and
184 here because their first mismatch is RV64 object-route stale live-value
consumption from `s2`, not prepared object-data/static-storage publication,
layout, initializer payload, symbol binding, or relocation.

## Suggested Next

Execute Step 1 of the active 666 runbook: refresh rows 183, 184, and 219
together, name the first callee-saved/live-value owner or split, and keep rows
183 and 184 out of static-storage object-data repair unless new evidence
contradicts the retired 663 findings.

## Watchouts

- Keep byval payloads, pointer-local lowering, static-storage object-data
  publication/layout/initializer/relocation repair, packed local member
  offsets, CLI dump formatting, AArch64 dispatch, generic RISC-V object
  emission, and LLVM torture work outside this plan unless focused evidence
  proves the same first owner.
- Do not change expectations, unsupported markers, allowlists, timeout policy,
  runtime policy, baseline accounting, or unrelated backend families.
- Reject fixed-register or named-row shortcuts; repair one general
  callee-saved/live-value rule only after the first owner is proven.
- `riscv64-linux-gnu-gcc` was not available during the retired 663 evidence
  packet, so there is no supplementary assembly-link runtime check for the
  passing `.s` output.

## Proof

Lifecycle-only switch. Before the switch, the focused retired-663 proof command
reproduced the known 4/6 pass state with rows 183 and 184 failing at runtime:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_cli_riscv64_prepared_object_data_static_local_storage_obj|backend_cli_riscv64_prepared_object_data_static_local_initialized_storage_obj|backend_codegen_route_riscv64_prepared_object_data_static_local_storage|backend_codegen_route_riscv64_prepared_object_data_static_local_initialized_storage|backend_obj_runtime_rv64_prepared_object_data_static_local_storage|backend_obj_runtime_rv64_prepared_object_data_static_local_initialized_storage)$' > test_before.log 2>&1`

After the switch, the plan owner ran the same command into `test_after.log` and
compared against `test_before.log` for the retirement guard.

Regression guard comparison:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: PASS. Before and after both reported 4 passed, 2 failed, 6 total; no
new failing tests and no new suspicious >30s tests.
