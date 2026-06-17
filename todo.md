Status: Active
Source Idea Path: ideas/open/298_rv64_direct_scalar_call_boundary_hardening.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Add one neighboring direct scalar call case if still in scope

# Current Packet

## Just Finished

Step 4 of `plan.md`: classified and registered one neighboring direct scalar
call runtime case.

`local_arg_call.c` stays inside the active boundary: it uses one direct
same-module scalar helper call, one scalar integer register argument sourced
from a local `int`, and one scalar integer return. It does not require indirect
calls, varargs, stack-passed arguments, aggregate ABI, global memory, or
pointer-heavy memory. `two_arg_local_arg.c` was also inspected and remains a
neighboring in-scope candidate, but this packet registered only the smaller
single-argument local case.

Registered `backend_rv64_runtime_local_arg_call` with expected qemu exit `6`.
The rv64 emitter now reuses prepared call-plan metadata for direct GPR
register-source argument moves and direct GPR register-result publication, with
a fallback from the indexed argument lookup to the owning prepared call plan's
argument vector. Unsupported call forms still fail closed through the existing
direct-call guards.

## Suggested Next

Execute Step 5: run acceptance validation and document residual direct scalar
call boundaries.

## Watchouts

- Keep the route limited to direct scalar register calls and scalar integer
  returns.
- Do not accept indirect calls, varargs, stack-passed arguments, aggregate ABI,
  global function pointer tables, or pointer-heavy memory in this plan.
- The direct-call emission now fails closed without prepared call metadata and
  rejects unsupported ABI forms before runnable assembly is emitted.
- Use prepared call metadata and value homes; do not match helper names,
  filenames, or exact source shapes.
- `two_arg_local_arg.c` remains an unregistered neighboring candidate. It should
  only be added if the next route still wants another direct scalar register
  call case.
- Keep review artifacts under `review/` untouched unless a supervisor-provided
  lifecycle task explicitly requires reading or rewriting around one.

## Proof

Command:
`bash -o pipefail -c "{ cmake --build --preset default && ctest --test-dir build -R '^backend_rv64_runtime' --output-on-failure; } 2>&1 | tee test_after.log"`

Result: passed. Build succeeded and all 19 `backend_rv64_runtime` tests passed,
including new `backend_rv64_runtime_local_arg_call`.

Proof log: `test_after.log`.

Regression guard:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`

Result: passed with 18/18 before and 19/19 after.

RISC-V focused validation:

`ctest --test-dir build -R 'backend_riscv|backend_codegen_route_riscv64' --output-on-failure`

Result: 2/3 passed. Preserved pre-existing failure:
`backend_riscv_prepared_edge_publication`.
