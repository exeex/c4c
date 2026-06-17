Status: Active
Source Idea Path: ideas/open/300_rv64_direct_scalar_call_neighbor_coverage.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Harden direct scalar local-argument materialization

# Current Packet

## Just Finished

Step 2 of `plan.md` inspected the rv64 prepared-call/value-home path in
`src/backend/mir/riscv/codegen/emit.cpp` and found no additional compiler
hardening was needed for the registered direct scalar local-argument case.
`emit_riscv_simple_call` already routes prepared GPR call arguments through
the prepared call plan, and `emit_move_to_register` materializes a named `I32`
argument from a prepared 4-byte stack-slot value home with an `lw` before
placing it in the destination call register.

Generated assembly for `backend_rv64_runtime_two_arg_local_arg` stores local
`x`, reloads it with `lw`, moves it into `a0`, materializes `7` into `a1`,
calls `add_pair`, and saves/restores `ra` in `main`. No helper-name,
filename, or exact-source-shape shortcut was introduced.

## Suggested Next

Execute Step 3: inspect `tests/backend/case/two_arg_both_local_arg.c` and
`tests/backend/case/two_arg_second_local_arg.c`, registering only the adjacent
direct scalar local-argument cases that remain inside the prepared GPR
register-call contract.

## Watchouts

- Keep the route limited to direct scalar register calls with local scalar
  argument sources.
- Step 2 found the local scalar argument path already supported by prepared
  value-home metadata; no compiler code changed in this packet.
- Adjacent candidates should reuse the same prepared call/value-home route and
  stop if they expose stack arguments, aggregate ABI, pointer/member behavior,
  globals, indirect calls, or varargs.
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
