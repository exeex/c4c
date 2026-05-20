Status: Active
Source Idea Path: ideas/open/349_aarch64_recursive_call_argument_preservation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Recursive Call Preservation Boundary

# Current Packet

## Just Finished

Step 1 of `plan.md` localized the first bad boundary to missing post-call
reload/publication from prepared preservation homes into later call operands.

Representative evidence:

- `00181`/`Hanoi`: prepared BIR records four callsites and knows the incoming
  arguments cross calls. At the first recursive call (`block=2 inst=1`) it
  preserves `%p.n` in `slot#72+stack0`, `%p.source` in callee-saved `x20`,
  `%p.dest` in callee-saved `x21`, and `%p.spare` in `slot#73+stack8`; the
  later `Move` and second recursive `Hanoi` still publish arguments from ABI
  registers (`x1`, `x2`, `x3`, `w0`) instead of those homes. Generated
  `build/c_testsuite_aarch64_backend/src/00181.c.s` lines 349-364 show
  `bl Hanoi`, then `mov x0, x1`, `mov x1, x2`, `bl Move`, then
  `sub w13, w0, #1` and more argument moves from `x1`/`x2`/`x3`.
- `00176`/`quicksort`: prepared BIR records `%p.left` and `%p.right` as
  crossing calls and preserved in callee-saved `x20`/`x21`; for the second
  recursive call (`block=2 inst=7`) the call argument plan still says
  `arg1 from=register:x1 to=x1`. Generated
  `build/c_testsuite_aarch64_backend/src/00176.c.s` lines 668-676 show the
  first recursive `bl quicksort`, then the second call uses `mov w1, w1`
  for the original `right` argument.

Source boundary inspected:

- `src/backend/prealloc/call_plans.cpp` builds correct preservation facts for
  live call-crossing values.
- `src/backend/prealloc/regalloc.cpp` prefers callee-saved registers for
  values marked `crosses_call`.
- `src/backend/mir/aarch64/codegen/calls.cpp` lowers the prepared before-call
  move bundle.
- `src/backend/mir/aarch64/codegen/dispatch.cpp` clears call-clobbered emitted
  scalar registers after `bl`, but the next call setup does not reload or
  republish the preserved homes before consuming those values.

## Suggested Next

Execute Step 2 by adding focused backend coverage for an ordinary nested call
where a source argument is used after an intervening call. The test should fail
when post-call call-argument publication reads only the old ABI argument
register and should pass only when the later operand is reloaded from the
prepared stack slot/callee-saved home or otherwise republished through the
prepared value-home contract.

## Watchouts

- Do not special-case `00176`, `00181`, `quicksort`, `Hanoi`, one argument
  index, one register name, or one emitted `bl` neighborhood.
- The first bad boundary is not missing preservation metadata: the prepared
  call plans already record preserved values and routes. The implementation
  packet should target the handoff from those prepared homes to post-call
  operand publication/call-argument setup.
- Do not reopen indexed aggregate address/writeback, variadic/byval
  publication, frame layout, scalar publication, runner behavior, expectation
  policy, unsupported classification, CTest registration, timeout policy, or
  proof-log behavior without fresh first-bad-fact evidence.
- Keep routine progress in `todo.md`; edit `plan.md` only if the active
  runbook contract changes.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_return_lowering|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c)$' | tee test_after.log`

Result: build was up to date; 4/6 tests passed. The focused backend
guardrails passed. `c_testsuite_aarch64_backend_src_00176_c` still reports a
runtime mismatch, and `c_testsuite_aarch64_backend_src_00181_c` still exits
with a segmentation fault. Proof log: `test_after.log`.
