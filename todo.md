Status: Active
Source Idea Path: ideas/open/349_aarch64_recursive_call_argument_preservation.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Step 3/4 follow-up: Repair stale non-call scalar publication after preserved-home population

# Current Packet

## Just Finished

Step 3/4 follow-up repaired stale non-call scalar publication after
callee-saved preservation-home population.

Completed work:

- Added focused AArch64 dispatch coverage for a prepared value whose canonical
  home remains the stale incoming ABI register (`x1`) while an earlier
  clobbering call established a callee-saved preservation home (`x20`); the
  later scalar ALU use now consumes the preserved `x20` home.
- AArch64 call lowering now republishes established callee-saved preservation
  homes into block scalar state after a clobbering call when a later non-call
  use occurs before the next call, and at successor block entry when a prior
  call in function order established the home and the block has an early
  non-call use.
- `00176` advanced past the previous stale non-call publication: generated
  `build/c_testsuite_aarch64_backend/src/00176.c.s` now has
  `mov x20, x20` at `for.cond.2` entry followed by `cmp w13, w20`, instead of
  the prior `mov w20, w1` before the compare.
- `00176` still fails with a segmentation fault. The new first bad fact is a
  later cross-block call-argument publication in `partition` block `block_3`:
  the final `swap` still emits `mov w0, w1` before `bl swap` for `%p.right`
  instead of publishing from the already-populated preserved `x20` home.
- `00181` still fails with the previously classified out-of-scope
  symbol/local publication crash.

## Suggested Next

For idea 349, repair cross-block call-argument publication after a prior
callee-saved preservation home has been established so later calls in successor
blocks use the preserved home instead of stale caller-ABI source registers.

## Watchouts

- Do not special-case `00176`, `00181`, `quicksort`, `Hanoi`, one argument
  index, one register name, or one emitted `bl` neighborhood.
- Non-call scalar publication now handles the observed same-block and
  successor-block cases by seeding block-local scalar state; do not widen this
  into a global "any prior preservation always dominates" rule without fresh
  dominance evidence.
- The population skip currently treats any earlier prepared preservation in
  function order as establishing the home; revisit with dominance-aware facts
  if a later packet needs broader CFG precision.
- `00176`'s current first bad fact is call-argument publication, not non-call
  scalar publication: block `block_3` final `swap` uses stale `w1` for
  `%p.right` even though `x20` was populated earlier.
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
`00176` advanced past stale non-call scalar publication from `w1`; the next
first bad fact is stale cross-block call-argument publication from `w1` before
the final `swap`. `00181` remains at the previously classified out-of-scope
stack-preserved symbol/local publication crash. Proof log: `test_after.log`.

Supervisor broader guard:

`ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed, 141/141.
