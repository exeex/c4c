Status: Active
Source Idea Path: ideas/open/298_rv64_direct_scalar_call_boundary_hardening.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Verify call-frame preservation boundaries

# Current Packet

## Just Finished

Step 3 of `plan.md`: verified and tightened rv64 call-frame preservation for
functions that contain direct calls.

The existing direct-call runtime case already saved/restored `ra`, and the
call-only path still emits the same `sd ra, 8(sp)` / `ld ra, 8(sp)` frame shape.
The frame-boundary gap was the mixed case where a function has both direct calls
and prepared local/saved-register stack metadata: rv64 emission previously
rejected that combination before a complete body. The emitter now computes a
call-frame `ra` spill slot after the prepared frame and any prepared saved
callee-register stack placements, fails closed when those placements are
incomplete or out of signed-12-bit stack offset range, and threads the chosen
offset through the return epilogue.

## Suggested Next

Execute Step 4: inspect a neighboring direct scalar call candidate, such as
`local_arg_call.c` or `two_arg_local_arg.c`, and register only one case that
stays inside this plan's direct scalar register-call boundary.

## Watchouts

- Keep the route limited to direct scalar register calls and scalar integer
  returns.
- Do not accept indirect calls, varargs, stack-passed arguments, aggregate ABI,
  global function pointer tables, or pointer-heavy memory in this plan.
- The direct-call emission now fails closed without prepared call metadata and
  rejects unsupported ABI forms before runnable assembly is emitted.
- Use prepared call metadata and value homes; do not match helper names,
  filenames, or exact source shapes.
- A manual `local_arg_call.c` rv64 probe now gets past the prior prepared
  frame-plus-`ra` boundary and places `ra` after prepared stack ranges, but the
  file is still unregistered and still needs Step 4 classification before any
  runtime expectation is added.
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
