Status: Active
Source Idea Path: ideas/open/13_aarch64_cts_00181_recursion_global_array_runtime.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reproduce And Localize Runtime Mismatch

# Current Packet

## Just Finished

Step 1 reproduced `c_testsuite_aarch64_backend_src_00181_c` as a runtime
segfault and localized the first bad owner to AArch64 call-boundary argument
lowering. BIR for `Hanoi` is correct, but final AArch64 reuses caller-saved
`x3` for the later `spare` argument after intervening calls instead of
reloading the prepared stack-preserved `%p.spare` value from `sp+8`.

## Suggested Next

Delegate Step 2 to create a focused recursive-call case where a fourth
pointer/int argument must survive two calls and then be republished as a later
call argument, independent of `00181` names or global array layout.

## Watchouts

Observed fault: `Move` traps at `ldr w9, [x10]` with `x0 == 0`; the null
source pointer comes from the third recursive `Hanoi` call path. Prepared BIR
records `%p.spare` as `route=stack_slot slot#73 stack+8` and the before-call
bundle for block 2 inst 4 says `move from_value_id=104 ... abi_index=1 ...
reason=call_arg_stack_to_register`, but emitted assembly in `Hanoi` contains
`mov x1, x3` after prior calls clobber `x3`. `PrintAll` global array reads are
lowered through constant-offset selects, but they are not the first crash.
Do not reopen the old prior-preservation selected fallback, weaken
c_testsuite expectations, or special-case `00181`, Tower of Hanoi symbols, or
one exact global array layout.

## Proof

Ran:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00181_c$') > test_after.log 2>&1`

Result: build succeeded; the single test failed with `[RUNTIME_NONZERO]` and
`exit=Segmentation fault`. Proof log: `test_after.log`.
