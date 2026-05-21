Status: Active
Source Idea Path: ideas/open/349_aarch64_recursive_call_argument_preservation.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Step 3/4 follow-up: Repair preservation-home population for in-scope 00176 residual

# Current Packet

## Just Finished

Step 3/4 follow-up repaired callee-saved preservation-home population for the
in-scope `00176` residual.

Completed work:

- Added focused AArch64 dispatch coverage that fails unless a prepared
  callee-saved preservation home is populated before the clobbering call and
  later reused after that call.
- AArch64 before-call lowering now emits semantic source-to-callee-saved-home
  population moves for prepared callee-saved preserved values when no earlier
  prepared preservation has already established that home in the function.
- `00176` advanced: generated
  `build/c_testsuite_aarch64_backend/src/00176.c.s` now emits `mov w20, w0`
  and `mov w21, w1` before `bl partition` in `quicksort`, and `partition`'s
  first `swap` call similarly populates `w21`/`w20` before the call.
- `00176` still fails with a segmentation fault. The new first bad fact is a
  later non-call scalar use in `partition`: generated assembly still reads
  `%p.right` through stale `w1` after `bl swap` in the loop/exit path, e.g.
  `mov w20, w1` before the loop compare and `mov w0, w1` before the final
  `swap`, instead of publishing from the already-populated preserved `x20`
  home.
- `00181` still fails with the previously classified out-of-scope
  symbol/local publication crash.

## Suggested Next

For idea 349, repair non-call scalar publication after a clobbering call so
uses of a prepared preserved value reload from the established callee-saved
home instead of stale caller-ABI source registers.

## Watchouts

- Do not special-case `00176`, `00181`, `quicksort`, `Hanoi`, one argument
  index, one register name, or one emitted `bl` neighborhood.
- Preservation-home population is now present for the first clobbering call,
  but later non-call consumers can still read stale source homes unless scalar
  publication also consults the preserved home.
- The population skip currently treats any earlier prepared preservation in
  function order as establishing the home; revisit with dominance-aware facts
  if a later packet needs broader CFG precision.
- `00181` still has likely in-scope `Hanoi` call-preservation hazards after
  the early `main`/`PrintAll` crash is removed, including `Hanoi` lines
  352-367 where `%p.n` and `%p.spare` have prepared stack-slot preservation
  routes. Do not treat that later neighborhood as the current first bad fact
  until the early symbol/local publication crash is fixed or bypassed by a
  separate owner.
- Do not reopen indexed aggregate address/writeback, variadic/byval
  publication, frame layout, scalar publication, runner behavior, expectation
  policy, unsupported classification, CTest registration, timeout policy, or
  proof-log behavior without fresh first-bad-fact evidence.
- Keep routine progress in `todo.md`; edit `plan.md` only if the active
  runbook contract changes.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_return_lowering|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c)$' | tee test_after.log`

Result: build completed; 4/6 tests passed. The focused backend guardrails
passed. `c_testsuite_aarch64_backend_src_00176_c` and
`c_testsuite_aarch64_backend_src_00181_c` still fail with segmentation faults.
`00176` advanced past missing preservation-home population to stale non-call
scalar publication from `w1`; `00181` remains at the previously classified
out-of-scope stack-preserved symbol/local publication crash. Proof log:
`test_after.log`.

Supervisor broader guard:

`ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed, 141/141.
