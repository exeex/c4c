Status: Active
Source Idea Path: ideas/open/349_aarch64_recursive_call_argument_preservation.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Representatives And Reclassify Residuals

# Current Packet

## Just Finished

Step 4 of `plan.md` classified the new `00176` and `00181` AArch64
segmentation faults after the post-call preserved-argument publication repair.

Completed work:

- `00176` still fails with a segmentation fault, but it has advanced past the
  stale second-recursive-call argument publication failure. Generated
  `build/c_testsuite_aarch64_backend/src/00176.c.s` lines 668-676 publish the
  first later recursive call from `x20` and the second call's preserved
  `right` argument from `x21`.
- The new `00176` first bad fact remains in-scope for idea 349. Prepared
  `/tmp/00176.prepared.txt` records `quicksort` callsites preserving
  `%p.left#214` in `x20` and `%p.right#215` in `x21`, but the generated
  `quicksort` prologue only saves/restores `x20`/`x21` and never initializes
  those prepared homes from incoming `w0`/`w1` before using them after
  `bl partition`. Runtime gdb evidence shows unbounded recursive
  `quicksort` frames ending in `swap` with corrupt arguments
  `w0=0xaaac229f`, `w1=0xf7ff202f`, consistent with missing preservation-home
  population rather than the old stale call-argument publication.
- `00181` also still fails with a segmentation fault, and it advanced past the
  previously observed `Move` call publication neighborhood. Generated
  `build/c_testsuite_aarch64_backend/src/00181.c.s` lines 358-360 still use
  `mov x0, x20` and `mov x1, x21` for `Move` after the first recursive
  `Hanoi` call.
- The current `00181` first crash is outside idea 349's recursive-call
  preservation scope. It occurs before `Hanoi`: generated `main` lines 566-569
  materialize `.str8` in `x0` and then immediately overwrite `x0` with
  `ldr x0, [sp, #8]` before `printf`; lines 571-579 similarly materialize
  `A`/`B`/`C` and then overwrite `x1`/`x2`/`x3` from stack slots before
  `Hanoi`. Prepared `/tmp/00181.prepared.txt` classifies `@.str8`, `@A`, `@B`,
  and `@C` as stack-slot preserved values, but the generated code consumes
  uninitialized stack slots instead of the materialized symbol addresses. gdb
  stops in `printf`/`strchrnul` with invalid format pointer
  `x0=0x6c626f725020696f`, so this residual is local/symbol-address
  publication from stack-preserved homes, not a post-recursive-call reload.

## Suggested Next

For idea 349, repair the general callee-saved preservation-home population path
for live values that are assigned to `PreparedCallPreservedValue` homes before
the clobbering call, using `00176` as the representative and focused coverage
that is independent of quicksort.

## Watchouts

- Do not special-case `00176`, `00181`, `quicksort`, `Hanoi`, one argument
  index, one register name, or one emitted `bl` neighborhood.
- A repair that only changes later call-argument lookup is no longer enough
  for `00176`; the missing step is making the prepared callee-saved home
  actually contain the source value before the call that relies on it.
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
`c_testsuite_aarch64_backend_src_00181_c` both still fail with segmentation
faults after advancing past the later-call callee-saved argument publication
boundary. `00176` remains in-scope as missing prepared preservation-home
population; `00181`'s current first crash is outside scope as stack-preserved
symbol/local publication. Proof log: `test_after.log`.
