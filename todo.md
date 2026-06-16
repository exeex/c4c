Status: Active
Source Idea Path: ideas/open/296_rv64_runtime_scalar_local_foundation.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Focused Backend Validation

# Current Packet

## Just Finished

Step 5 from `plan.md` completed: fresh focused validation reran the rv64
runtime subset plus the focused RISC-V/backend route subset.

All 14 `backend_rv64_runtime_*` cases pass under qemu, including the new
scalar and local-slot cases. The focused widened subset remains monotonic
against the accepted baseline, with only the known pre-existing
`backend_riscv_prepared_edge_publication` failure.

## Suggested Next

Supervisor should ask the plan owner to decide whether to close the active
runbook, preserving the known `backend_riscv_prepared_edge_publication`
baseline caveat as outside this rv64 runtime slice unless separately repaired.

## Watchouts

- Keep qemu runtime execution as the acceptance boundary.
- Do not accept BIR or LLVM fallback text as RISC-V assembly.
- Do not add testcase-name or exact-source-shape shortcuts.
- `EXPECTED_RUN_CODE` is a generic harness contract for qemu process status; it
  should not be used to hide wrong semantics.
- The local-slot path is intentionally narrow: non-volatile same-width i32
  frame-slot load/store using prepared memory-access and frame-plan facts.
- Do not expand this slice into arrays, aggregates, globals, pointer
  provenance, dynamic stack, or calls plus local frames without a separate
  packet.
- The supervisor-noted baseline failure
  `backend_riscv_prepared_edge_publication` still appears in the delegated
  proof subset.

## Proof

Passed:
`bash -o pipefail -c 'cmake --build --preset default && ctest --test-dir build -R '\''^backend_rv64_runtime|backend_riscv|backend_codegen_route_riscv64'\'' --output-on-failure' 2>&1 | tee test_after.log`

Accepted with baseline caveat:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Fresh rv64 runtime subset:
`ctest --test-dir build -R '^backend_rv64_runtime' --output-on-failure`

Proof log: `test_after.log`.
