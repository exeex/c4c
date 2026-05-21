Status: Active
Source Idea Path: ideas/open/349_aarch64_recursive_call_argument_preservation.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Step 3/4 follow-up: Repair stale cross-block call-argument publication from preserved homes

# Current Packet

## Just Finished

Step 3/4 follow-up repaired stale cross-block call-argument publication from
callee-saved preservation homes.

Completed work:

- Broadened AArch64 prior-preserved call-argument lookup from same-block-only
  calls to earlier calls in function order, matching the existing preserved-home
  population skip rule.
- Added focused two-block AArch64 dispatch coverage where an entry-block call
  preserves `%arg` into `x20`, then a successor-block call must publish `%arg`
  from `x20` instead of the stale original `x1` home.
- `00176` advanced past the delegated final `swap` stale publication:
  generated `build/c_testsuite_aarch64_backend/src/00176.c.s` now prints
  `mov x0, x20` before the final `bl swap` in `partition` block `block_3`, and
  `mov w0, w1` no longer appears in the generated file.
- `00176` still fails with a segmentation fault. The new first bad fact appears
  to be generated control-flow/label emission in `partition`: prepared metadata
  has `block_3` as a return block followed by `block_4` and `block_5`, but the
  generated assembly prints the `block_3` return/epilogue at `.LBB90_4` before
  the later `swap` code, and that later code has no visible block label before
  it. Concrete evidence: prepared dump reports `block block_3 terminator=return`,
  `block block_4 terminator=branch target=block_5`, and
  `block block_5 terminator=branch target=for.latch.2`; generated assembly
  around `build/c_testsuite_aarch64_backend/src/00176.c.s` lines 624-637 prints
  `mov x0, x20`, `bl swap`, then return/epilogue, then an unlabeled later
  `mov w0, w13` / `bl swap` sequence before `.LBB90_7`.
- `00181` still fails with the previously classified out-of-scope
  symbol/local publication crash.

## Suggested Next

For idea 349, hand off to the owner of AArch64 function/block emission order or
label placement for `partition`-style multi-block control flow, using the
prepared/generated evidence above to decide whether this belongs in the active
runbook or a separate lifecycle slice.

## Watchouts

- Do not special-case `00176`, `00181`, `quicksort`, `Hanoi`, one argument
  index, one register name, or one emitted `bl` neighborhood.
- Non-call scalar publication now handles the observed same-block and
  successor-block cases by seeding block-local scalar state; do not widen this
  into a global "any prior preservation always dominates" rule without fresh
  dominance evidence.
- Call-argument publication now uses the same earlier-in-function-order rule as
  preservation-home population. Revisit both with dominance-aware facts if a
  later packet needs broader CFG precision.
- `00176`'s stale final `swap` publication is no longer the current first bad
  fact; avoid reworking call-argument publication again unless fresh evidence
  shows another stale preserved-home source.
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
`00176` advanced past stale cross-block call-argument publication from `w1`;
the next first-bad fact appears to be generated AArch64 block label/emission
ordering after `partition` block `block_3`'s return. `00181` remains at the
previously classified out-of-scope stack-preserved symbol/local publication
crash. Proof log: `test_after.log`.

Supervisor broader guard:

`ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed, 141/141.
