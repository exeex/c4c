Status: Active
Source Idea Path: ideas/open/300_rv64_direct_scalar_call_neighbor_coverage.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect candidate cases and register the first in-scope target

# Current Packet

## Just Finished

Step 1 of `plan.md` inspected `tests/backend/case/two_arg_local_arg.c` and
confirmed it is in scope: a direct scalar two-argument call where the first
argument is sourced from a local `int` and the second is an immediate. The case
was registered as `backend_rv64_runtime_two_arg_local_arg` with expected qemu
process status `12`.

Observed rv64 lowering stores the local scalar, reloads it, moves it into
`a0`, materializes `7` into `a1`, calls `add_pair`, and saves/restores `ra` in
`main`. The runtime runner passed the case under qemu and therefore did not
accept BIR or LLVM fallback text as success.

## Suggested Next

Execute Step 2: keep `backend_rv64_runtime_two_arg_local_arg` green while
inspecting the prepared-call/value-home path for direct scalar local argument
materialization in `src/backend/mir/riscv/codegen/emit.cpp`.

## Watchouts

- Keep the route limited to direct scalar register calls with local scalar
  argument sources.
- `two_arg_local_arg.c` passed by registration only; no compiler change was
  needed in this packet.
- Do not absorb pointer parameters, member indexing, indirect calls, varargs,
  stack-passed arguments, aggregate ABI, globals, or function pointer tables
  into this plan.
- Use prepared call plans and value homes; do not match helper names,
  filenames, or exact source shapes.
- Keep review artifacts under `review/` untouched unless a supervisor-provided
  lifecycle task explicitly requires reading or rewriting around one.

## Proof

Ran exactly:

`{ cmake --build --preset default; ctest --test-dir build -j --output-on-failure -R '^backend_rv64_runtime'; } > test_after.log 2>&1`

Result: pass. `backend_rv64_runtime_two_arg_local_arg` passed, and the full
rv64 runtime subset passed: 21 tests, 0 failures. Proof log:
`test_after.log`.
