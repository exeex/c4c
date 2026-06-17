Status: Active
Source Idea Path: ideas/open/300_rv64_direct_scalar_call_neighbor_coverage.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add adjacent direct scalar local-argument coverage

# Current Packet

## Just Finished

Step 3 of `plan.md` inspected `tests/backend/case/two_arg_both_local_arg.c`
and `tests/backend/case/two_arg_second_local_arg.c`. Both candidates stay
inside the bounded direct scalar register-call contract: they call
`add_pair(int, int)` directly, source only local `int` scalar values and
integer literals, and do not involve stack-passed arguments, aggregate ABI,
pointers, members, globals, indirect calls, or varargs.

Registered both in-scope candidates with
`c4c_add_backend_rv64_runtime_case(...)` in `tests/backend/CMakeLists.txt`
using expected qemu status `12`. No compiler change was needed, and no
helper-name, filename, or exact-source-shape shortcut was introduced.

## Suggested Next

Execute Step 4: run the broader acceptance validation for the broadened rv64
direct scalar call surface and document the final in-scope/out-of-scope
boundaries.

## Watchouts

- Keep any Step 4 validation focused on the broadened direct scalar register
  call surface unless the supervisor explicitly broadens the proof.
- Step 3 found both adjacent candidates in scope; there were no rejected
  candidate boundaries in this packet.
- The local scalar argument path remained supported by prepared value-home
  metadata; no compiler code changed in this packet.
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

Result: pass. `backend_rv64_runtime_two_arg_both_local_arg` and
`backend_rv64_runtime_two_arg_second_local_arg` both passed, and the full rv64
runtime subset passed: 23 tests, 0 failures. Proof log: `test_after.log`.
