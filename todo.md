Status: Active
Source Idea Path: ideas/open/298_rv64_direct_scalar_call_boundary_hardening.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Harden direct scalar argument and return lowering

# Current Packet

## Just Finished

Step 2 of `plan.md`: inspected and hardened the rv64 direct scalar call
argument/result lowering path used by `backend_rv64_runtime_two_arg_helper`.

The existing path already moved scalar arguments through prepared value homes
and copied scalar results from the prepared result register, but it still had
fallback defaults when authoritative prepared call metadata was missing. The
rv64 emitter now requires a matching `PreparedCallPlan`, direct fixed-arity or
same-module callee metadata, no indirect/variadic/stack-argument/memory-return
plan, one GPR register lane per argument, and one GPR register result lane when
the BIR call has a result.

## Suggested Next

Execute Step 3: verify call-frame preservation boundaries for functions that
contain direct calls, especially `ra` save/restore behavior.

## Watchouts

- Keep the route limited to direct scalar register calls and scalar integer
  returns.
- Do not accept indirect calls, varargs, stack-passed arguments, aggregate ABI,
  global function pointer tables, or pointer-heavy memory in this plan.
- The direct-call emission now fails closed without prepared call metadata and
  rejects unsupported ABI forms before runnable assembly is emitted.
- Use prepared call metadata and value homes; do not match helper names,
  filenames, or exact source shapes.
- Keep review artifacts under `review/` untouched unless a supervisor-provided
  lifecycle task explicitly requires reading or rewriting around one.
- Step 1 did not need `local_arg_call.c`; it remains a possible neighboring
  direct scalar call candidate for a later packet.

## Proof

Command:
`bash -o pipefail -c "{ cmake --build --preset default && ctest --test-dir build -R '^backend_rv64_runtime' --output-on-failure; } 2>&1 | tee test_after.log"`

Result: passed. Build succeeded and all 18 `backend_rv64_runtime` tests passed,
including `backend_rv64_runtime_two_arg_helper`.

Proof log: `test_after.log`.

Regression guard:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: passed with 18/18 before and 18/18 after.

RISC-V focused validation:

`ctest --test-dir build -R 'backend_riscv|backend_codegen_route_riscv64' --output-on-failure`

Result: 2/3 passed. Preserved pre-existing failure:
`backend_riscv_prepared_edge_publication`.
