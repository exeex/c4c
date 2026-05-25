Status: Active
Source Idea Path: ideas/open/13_aarch64_cts_00181_recursion_global_array_runtime.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Validate The 00181 Family

# Current Packet

## Just Finished

Step 2 added focused backend coverage for a caller-saved value originally in
`x3` that is stack-preserved across a call and later republished as ABI
argument `x1`; the focused dispatch test passes and rejects stale `mov x1, x3`
for that reduced same-feature shape. The remaining production/fixture split was
the dispatch bridge retargeting an explicit stack `PriorPreservation` source
back to a stale emitted scalar register for the same value name; the
call-argument stack-preserved source now remains a frame-slot source through
that retargeting pass. `00181` now emits `ldr x1, [sp, #8]` before the later
recursive `Hanoi` call instead of `mov x1, x3`.

## Suggested Next

Run Step 4's remaining nearby same-feature validation needed to reject a
named-test-only fix beyond the already-green `00181` and focused backend/unit
proof.

## Watchouts

Prepared BIR for `Hanoi` shows `%p.spare#104` as a stack-slot preserved value
for block 2 calls at inst 1, inst 2, and inst 4, and the inst 4 argument plan
says arg index 1 uses `source_reg=x3` and `dest_reg=x1`. The owner ambiguity
resolved to explicit stack `PriorPreservation` call-argument sources carrying a
`result_value_name`; `calls_dispatch_bridge.cpp` uses that name to retarget
call-boundary memory sources to previously emitted scalar registers, which was
correct for ordinary scalar producers but wrong for stack-preserved
caller-saved values after an intervening call. The repair keeps only
stack-prior-preservation call arguments non-retargetable by clearing the memory
source value name while preserving the prepared value id and frame-slot facts.
Do not special-case `00181`, Tower of Hanoi symbols, or one exact global array
layout.

## Proof

Ran:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00181_c)$') > test_after.log 2>&1`

Result: build succeeded; `backend_aarch64_instruction_dispatch` passed,
including the new focused same-feature coverage, and
`c_testsuite_aarch64_backend_src_00181_c` passed. Proof log:
`test_after.log`.
