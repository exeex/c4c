Status: Active
Source Idea Path: ideas/open/654_direct_global_stack_backed_pointer_branch_boundary.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Representative Integration And Regression Proof

# Current Packet

## Just Finished

Step 4 of `plan.md`: re-ran representative RV64 integration evidence for
`tests/c/external/gcc_torture/src/20000314-3.c` and the focused direct-global
stack-backed pointer branch object-emission guard.

Changed files:
- `todo.md`

Representative evidence:
- Semantic BIR still has the target branch in `attr_rtx`: `%t1 = bir.ne ptr
  %p.varg0, @arg0`.
- Prepared BIR for `attr_rtx` still publishes the RHS branch stack-load
  authority: `role=rhs value=@arg0 value_id=4 policy=load_from_stack_slot
  pointer_status=proven status=available source_freshness_status=selected`.
- Prepared BIR still publishes direct-global identity for that branch operand:
  `address_materialization block=entry inst_index=0 kind=direct_global
  result=@arg0 symbol=arg0 policy=direct offset=0`.
- The representative object route now stops before the branch boundary at the
  known upstream owner:
  `unsupported_call_abi: RV64 object route requires supported ordinary
  same-module call ABI/result lowering; function=attr_eq; block=entry;
  instruction_index=2; callee=attr_rtx`.
- The focused direct-global object-emission guard still has no direct-global
  pointer branch failure output. Its test binary fails on existing unrelated
  local-memory/byval/layout checks.

## Suggested Next

Supervisor should compare this `test_after.log` against the current backend
baseline and decide whether the Step 3/Step 4 direct-global branch slice can be
accepted with a non-decreasing baseline, or whether broad backend baseline
cleanup is required first.

## Watchouts

- The current representative blocker is the known upstream
  `unsupported_call_abi`, not a direct-global branch regression.
- The direct-global branch boundary cannot be reached through the full
  `src/20000314-3.c` object route until same-module call ABI/result lowering is
  handled elsewhere.
- No expectation files, allowlists, timeout/accounting files, or ABI lowering
  code were changed in this packet.

## Proof

Focused probes:
- `./build/c4cll -I tests/c/external/gcc_torture --dump-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000314-3.c`
  succeeded; evidence saved under `build/agent_state/654_step4_20000314-3.bir.*`.
- `./build/c4cll -I tests/c/external/gcc_torture --dump-prepared-bir --target riscv64-linux-gnu --mir-focus-function attr_rtx tests/c/external/gcc_torture/src/20000314-3.c`
  succeeded; evidence saved under
  `build/agent_state/654_step4_20000314-3.attr_rtx.prepared.*`.
- `./build/c4cll -I tests/c/external/gcc_torture --codegen obj --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000314-3.c -o build/agent_state/654_step4_20000314-3.o`
  failed at the known upstream `unsupported_call_abi` owner; evidence saved
  under `build/agent_state/654_step4_20000314-3.obj.*`.
- `cmake --build --preset default --target backend_riscv_object_emission_test`
  succeeded.
- `./build/tests/backend/mir/backend_riscv_object_emission_test` failed on
  existing unrelated checks; no direct-global pointer branch guard failure was
  reported in `build/agent_state/654_step4_backend_riscv_object_emission_test.out`.

Delegated proof:
- Command: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
- Log: `test_after.log`
- Result: failed. Build completed, then CTest reported 32 failures out of 365
  backend tests. The failing set matches the broad backend-baseline family
  already seen by Step 3: RV64 dump/runtime cases plus
  `backend_riscv_object_emission`, `backend_aarch64_instruction_dispatch`,
  `backend_prepare_liveness`, `backend_prepare_frame_stack_call_contract`,
  `backend_prepared_printer`, `backend_prealloc_inline_asm`, and three backend
  CLI prepared-BIR checks.

Supervisor regression comparison:
- Compared canonical `test_before.log` and `test_after.log` for the same
  backend subset using the non-decreasing guard. Both logs report 333 passed,
  32 failed, 365 total; no new failing tests and no newly slow tests.

Plan-owner lifecycle decision:
- Closure was considered for
  `ideas/open/654_direct_global_stack_backed_pointer_branch_boundary.md`
  after Steps 1-4 completed.
- Source-idea completion is treated as satisfied for this runbook: the
  direct-global stack-backed pointer branch boundary has focused semantic
  positive/negative coverage, and the representative full object route is now
  blocked earlier by out-of-scope `unsupported_call_abi` before reaching the
  branch boundary.
- Close gate rejected closure because
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`
  exited 1: both backend logs report 333 passed, 32 failed, 365 total, with no
  new failures, but the checker requires a strictly increased passed count.
- Lifecycle state remains active pending supervisor decision on whether to
  supply an acceptable close-gate proof, adjust the close-scope policy, or
  leave the runbook parked behind the current backend baseline.
