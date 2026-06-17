# RV64 Direct Scalar Call Boundary Hardening

## Goal

Turn the bounded direct scalar call support exposed by idea 296 into an
explicitly owned rv64 runtime capability, with qemu-backed tests and clear
fail-closed boundaries.

## Starting Point

Idea 296 selected `return_add.c`, which required a tiny direct-call bridge even
though broad function-call lowering was out of scope. Review artifacts judged
that implementation acceptable because it was bounded:

- direct, non-indirect calls only;
- up to eight register arguments;
- prepared call argument/result metadata;
- no stack arguments;
- no varargs;
- no aggregate returns or by-value aggregate ABI;
- no frame-plus-call widening beyond the selected narrow path.

This follow-up makes that scope explicit so future rv64 call work does not
continue as accidental expansion inside unrelated scalar/local ideas.

## Candidate Cases

Start from existing `tests/backend/case/` files. Suggested order:

1. `two_arg_helper.c` or another small direct scalar helper-call case whose
   expected process status is simple.
2. `local_arg_call.c` only if it stays within scalar register arguments and
   existing local value support.
3. `two_arg_local_arg.c` or neighboring two-arg scalar cases only after the
   first case proves the call boundary cleanly.

Avoid cases with indirect calls, varargs, aggregates, global function pointer
tables, stack-passed arguments, or pointer-heavy memory until separate ideas own
those surfaces.

## In Scope

- Add rv64 runtime tests using `c4c_add_backend_rv64_runtime_case(...)`.
- Harden direct scalar call lowering for register arguments and scalar integer
  returns.
- Preserve/verify `ra` save-restore behavior for functions containing direct
  calls.
- Fail closed for unsupported call forms, including indirect calls, stack
  arguments, variadic calls, aggregate arguments, and aggregate returns.
- Use prepared call plans/value homes as the source of truth.

## Out of Scope

- Indirect calls and function pointer calls.
- Varargs.
- Stack-passed arguments.
- Aggregate/byval/sret ABI.
- Global memory or function pointer tables.
- Broad ABI cleanup for non-rv64 backends.
- Filename-specific or exact-source-shape shortcuts.

## Acceptance Criteria

- At least one new direct scalar helper-call runtime case executes under
  `qemu-riscv64`.
- Existing rv64 runtime cases continue to pass:
  `ctest --test-dir build -R '^backend_rv64_runtime' --output-on-failure`.
- Unsupported call forms remain rejected/fail-closed rather than producing
  partial runnable assembly.
- Existing RISC-V focused tests remain monotonic:
  `ctest --test-dir build -R 'backend_riscv|backend_codegen_route_riscv64' --output-on-failure`
  either passes or preserves only documented baseline failures.
- Escalate to `ctest --test-dir build -R '^backend_' --output-on-failure` if
  call lowering touches shared backend routing, prepared call plans, or
  non-rv64 code.

## Completion Notes

Closed after the active runbook completed Step 5 validation.

Implemented and registered qemu-backed rv64 runtime support for:

- `two_arg_helper.c`
- `local_arg_call.c`

The accepted support is metadata-backed and limited to direct scalar register
calls: prepared call plans are required, unsupported indirect/variadic/stack
argument/memory return/aggregate/multi-lane forms reject before runnable
assembly, GPR argument moves and scalar result publication use prepared
call-plan/value-home facts, and call-containing prepared stack frames place the
`ra` spill without overlapping prepared local or saved-register ranges.

`two_arg_local_arg.c` remains an unregistered neighboring candidate. It is not
required for this idea's closure because `local_arg_call.c` already proves a
neighboring direct scalar local-argument path through the same semantic call
lowering path.

Validation recorded at close:

- Regression guard over canonical rv64 runtime logs passed:
  `test_before.log` had 18 passing tests and `test_after.log` had 19 passing
  tests, with no new failures.
- Step 5 rv64 runtime acceptance passed all 19
  `backend_rv64_runtime_*` cases.
- Step 5 RISC-V focused acceptance passed the two
  `backend_codegen_route_riscv64_*` tests and preserved the pre-existing
  `backend_riscv_prepared_edge_publication` failure.
- Reviewer report `review/rv64_call_step1_hook_review.md` judged the route
  aligned with this idea and not testcase-overfit after the first registered
  call case.

## Reviewer Reject Signals

- The route expands into indirect calls, varargs, stack args, or aggregate ABI
  without a new source idea.
- The implementation recognizes one helper function name or exact testcase
  shape instead of lowering from prepared metadata.
- The runtime proof skips qemu.
- The runner accepts BIR/LLVM fallback text.
- Existing backend tests are weakened or expectation-rewritten to hide a call
  boundary failure.

## Notes For The Agent

- Read the route reviews from 296 if they are available under `review/`; they
  identified direct-call support as the main scope watch.
- Keep this idea focused on direct scalar calls. If a candidate case requires
  memory or local-stack broadening, route that to idea 297 or a new split.
- Prefer adding explicit fail-closed checks for unsupported call forms before
  adding more accepted cases.
