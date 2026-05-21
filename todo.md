Status: Active
Source Idea Path: ideas/open/377_aarch64_external_libc_call_result_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Call-Result Publication

# Current Packet

## Just Finished

Step 1: Localize Call-Result Publication completed for `00187`.

First bad fact: semantic BIR is still correct at the `fread` site:
`%t17 = bir.call i64 fread(...)` immediately feeds
`%t19 = bir.ne i64 %t17, 6`. Prepared state also knows the ABI result source
and consumer home: `%t17` is value_id 11, `fread` has
`result bank=gpr from=register:x0 to=stack_slot:stack+96`, and the focused
prepared dump records `storage %t17 ... spill_slot=slot#19+stack96`.

The emitted assembly loses publication after `bl fread`: it emits arguments
through `x0`-`x3`, calls `bl fread`, then immediately emits
`ldr x9, [sp, #96]` / `cmp x9, #6` with no intervening store from `x0` to
`[sp,#96]`. So the result is represented correctly as the AAPCS64 `x0`
call-result plus a prepared stack home at slot #19/stack+96, but generated code
only consumes the stack home and never publishes `x0` into it.

Owning boundary: smallest likely repair owner is AArch64 MIR call-result
publication, specifically the after-call move lowering from
`PreparedMovePhase::AfterCall` / `CallResultAbi` to a stack-slot destination.
Prealloc/call-planning has already produced the needed fact; the AArch64 call
lowering path drops it before emission.

## Suggested Next

Start Step 2 by teaching AArch64 after-call lowering to materialize scalar GPR
call results from ABI `x0` into prepared stack-slot homes, then recheck that
`00187.c.s` contains a store to `[sp,#96]` before the compare or otherwise
retargets the consumer to the live published result.

## Watchouts

Do not special-case `00187`, `fread`, or `[sp,#96]`. The general condition is a
scalar external call result whose prepared destination home is a stack slot.
`src/backend/mir/aarch64/codegen/calls.cpp` currently returns `std::nullopt`
for this shape in `lower_after_call_move` when
`result_plan->destination_storage_kind == StackSlot`, which explains why no
call-boundary move is emitted. The consumer-side stale load is not introduced
by BIR or prealloc; it is a normal read of the prepared stack home after the
publication move was skipped.

## Proof

Ran exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00187_c$'; } > test_after.log 2>&1`

Result: failed as expected for the localized issue. Build was up to date; the
focused CTest reported a runtime mismatch where actual output contains the
extra line `couldn't read fred.txt`, confirming the stale post-`fread`
comparison. Proof log: `test_after.log`.
