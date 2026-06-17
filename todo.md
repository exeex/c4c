Status: Active
Source Idea Path: ideas/open/298_rv64_direct_scalar_call_boundary_hardening.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Harden direct scalar argument and return lowering

# Current Packet

## Just Finished

Step 1 of `plan.md`: selected `tests/backend/case/two_arg_helper.c` as the
first in-scope direct scalar helper-call runtime case and registered it as
`backend_rv64_runtime_two_arg_helper`.

Classification: in scope. The case is a direct non-indirect call to
`add_pair(int, int)`, passes two scalar integer register arguments, returns a
scalar integer, and has no varargs, stack-passed arguments, aggregate ABI,
globals, function pointer tables, or pointer-heavy memory. The C `main` return
value is `12`, so the rv64 runtime registration uses `EXPECTED_RUN_CODE 12`.

Observed behavior: the delegated rv64 runtime proof passed. The runner emitted
native RISC-V assembly rather than BIR/LLVM fallback text, clang linked it, and
`qemu-riscv64` returned the expected status `12`.

## Suggested Next

Execute Step 2: inspect prepared call metadata/value homes and harden the
direct scalar argument/result lowering path used by
`backend_rv64_runtime_two_arg_helper`.

## Watchouts

- Keep the route limited to direct scalar register calls and scalar integer
  returns.
- Do not accept indirect calls, varargs, stack-passed arguments, aggregate ABI,
  global function pointer tables, or pointer-heavy memory in this plan.
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
