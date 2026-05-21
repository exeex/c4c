Status: Active
Source Idea Path: ideas/open/349_aarch64_recursive_call_argument_preservation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Recursive/Nested Call Coverage and Repair Post-Call Value Consumption

# Current Packet

## Just Finished

Step 2 of `plan.md` added focused nested-call coverage and repaired
post-call call-argument publication for values with a prior prepared
preservation route.

Completed work:

- Added `nested_call_argument_publishes_from_prior_preservation_home`, a
  focused AArch64 dispatch test with two ordinary calls. The second call's
  source value starts in caller-clobbered `x1`, the first call records the
  value preserved in callee-saved `x20`, and the test requires the later call
  setup to emit `mov x0, x20` after the intervening `bl`.
- Updated AArch64 before-call move lowering so later call-argument moves look
  back to earlier same-block `PreparedCallPreservedValue` routes for the same
  prepared value id and publish from that callee-saved or stack preservation
  home.
- Preserved-source call operands intentionally do not carry the stale source
  value name into dispatch scalar retargeting, so a prepared preservation home
  is not rewritten back to a caller-clobbered emitted ABI register.
- Representative assembly advanced past the localized stale later-call
  argument issue: `00176` now uses `mov x1, x21` for the second recursive
  `quicksort` argument after the first `bl quicksort`; `00181` now uses
  `mov x0, x20` and `mov x1, x21` for the `Move` call after `bl Hanoi`.

## Suggested Next

Execute Step 3/4 follow-up classification for the remaining external
representative failures. The next packet should inspect the new segmentation
faults in `00176` and `00181` after the later-call callee-saved argument
publication fix, starting from the generated neighborhoods recorded below.

## Watchouts

- Do not special-case `00176`, `00181`, `quicksort`, `Hanoi`, one argument
  index, one register name, or one emitted `bl` neighborhood.
- `00176` advanced past the known second recursive stale-`right` argument
  failure, but still segfaults. Generated lines 668-676 now show
  `mov x0, x20`, `mov w1, w9`, `bl quicksort`, then `mov w0, w9`,
  `mov x1, x21`, `bl quicksort`.
- `00181` advanced the `source`/`dest` publication into `Move`, but still
  segfaults. Generated lines 352-367 still include likely stale scalar
  consumption around `%p.n`/`%p.spare`: `sub w13, w0, #1` before the first
  recursive call and another `sub w13, w0, #1` after `bl Move`, while `%p.n`
  is stack-preserved.
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
`c_testsuite_aarch64_backend_src_00181_c` both still fail with segmentation
faults after advancing past the later-call callee-saved argument publication
boundary. Proof log: `test_after.log`.

Supervisor broader guard after the post-call argument publication repair:

`ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed 141/141.
