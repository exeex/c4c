# RV64 Direct Scalar Call Neighbor Coverage

## Goal

Broaden the qemu-backed rv64 direct scalar call coverage from the initial
helper-call cases to neighboring local-argument shapes, while preserving the
bounded direct scalar register-call contract from idea 298.

## Starting Point

Idea 298 closed with qemu-backed support for:

- `two_arg_helper.c`;
- `local_arg_call.c`;
- direct, non-indirect scalar calls;
- register arguments and scalar integer returns;
- fail-closed rejection for unsupported call forms.

The natural next cases are small scalar helpers where one or both call
arguments originate from local scalar values.

## Candidate Cases

Suggested order:

1. `tests/backend/case/two_arg_local_arg.c`
2. `tests/backend/case/two_arg_both_local_arg.c`
3. `tests/backend/case/two_arg_second_local_arg.c`

Keep this idea to direct scalar calls. If a candidate drags in pointer
parameters, member indexing, stack arguments, varargs, or aggregate ABI, move it
to a separate idea.

## In Scope

- Register one or more candidate cases with
  `c4c_add_backend_rv64_runtime_case(...)`.
- Harden direct scalar call argument materialization when arguments come from:
  - rematerializable immediates;
  - prepared scalar local homes;
  - loaded local scalar values already supported by the rv64 emitter.
- Preserve/verify `ra` save-restore behavior for call-containing functions.
- Keep unsupported call forms fail-closed.
- Use prepared call plans/value homes as authority.

## Out of Scope

- Pointer parameters and member indexing. Route those to idea 299.
- Indirect calls and function pointer calls.
- Varargs.
- Stack-passed arguments.
- Aggregate/byval/sret ABI.
- Global memory or function pointer tables.
- Filename-specific or exact-source-shape shortcuts.

## Acceptance Criteria

- At least one new neighboring direct scalar local-argument case executes under
  `qemu-riscv64`.
- `ctest --test-dir build -R '^backend_rv64_runtime' --output-on-failure`
  passes with the new case or cases included.
- Unsupported call forms remain rejected/fail-closed rather than producing
  partial runnable assembly.
- Existing RISC-V focused tests remain monotonic:
  `ctest --test-dir build -R 'backend_riscv|backend_codegen_route_riscv64' --output-on-failure`
  either passes or preserves only documented baseline failures.
- Escalate to `ctest --test-dir build -R '^backend_' --output-on-failure` if
  call lowering touches shared backend routing, prepared call plans, or
  non-rv64 code.

## Reviewer Reject Signals

- The implementation recognizes one helper function name or exact testcase
  shape instead of lowering from prepared metadata.
- The route expands into pointer/member indexing, indirect calls, varargs,
  stack args, or aggregate ABI without a new source idea.
- The runtime proof skips qemu.
- The runner accepts BIR/LLVM fallback text.
- Existing backend tests are weakened or expectation-rewritten to hide a call
  boundary failure.

## Notes For The Agent

- This should be a small follow-on to idea 298, not a broad call ABI expansion.
- Prefer proving one new neighboring case cleanly over adding many registrations
  with hidden emitter drift.
- If `two_arg_local_arg.c` already passes by registration only, still run the
  focused validation and record that no compiler changes were needed.
