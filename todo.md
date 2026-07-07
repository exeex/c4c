Status: Active
Source Idea Path: ideas/open/575_rv64_pointer_arithmetic_lowering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Representative Route Advancement

# Current Packet

## Just Finished

`plan.md` Step 4 reran the `src/20000819-1.c` RV64 object route after the
Step 3 pointer-arithmetic diagnostic slice and recorded the result under
`build/agent_state/575_rv64_pointer_arithmetic_lowering/step4/src_20000819-1.c/`.

The route still fails before object completion, with exit code `1`, but the old
generic `unsupported_instruction_fragment` is gone for the representative first
failure. It is now narrowed to:
`unsupported_pointer_arithmetic: RV64 object route requires prepared pointer
arithmetic lowering for loaded pointer base plus scaled integer byte offset`.

The current first owner remains the same pointer arithmetic owner:
`function=foo`, `block=entry`, `instruction_index=7`,
`instruction_kind=BinaryInst`, `owner=ptr %t4`. The prepared dump confirms the
shape is `%t4 = bir.add ptr %t1, %t4.byte_offset`, where `%t1` is a loaded
pointer base and `%t4.byte_offset` is a scaled integer byte offset.

## Suggested Next

Proceed to Step 5 closure validation if the supervisor accepts diagnostic
narrowing as this runbook's completion path, or delegate a new implementation
packet for actual RV64 pointer-add lowering if the source idea should advance
beyond fail-closed classification before closure.

## Watchouts

- The representative route has not advanced to a later distinct owner; it is
  still stopped at the same pointer arithmetic owner, now with the narrowed
  diagnostic.
- The object route's `cmake -P` wrapper returned exit code `1` for the expected
  fail-closed compile failure.
- Do not claim runtime/object-route success from this packet; it proves
  diagnostic narrowing only.
- If adding real lowering next, preserve the result owner publication path for
  register and stack homes.

## Proof

Ran the delegated route proof saved in:
`build/agent_state/575_rv64_pointer_arithmetic_lowering/step4/src_20000819-1.c/object-route.cmd`

Result: exit code `1`, recorded in:
`build/agent_state/575_rv64_pointer_arithmetic_lowering/step4/src_20000819-1.c/object-route.rc`

Route log:
`build/agent_state/575_rv64_pointer_arithmetic_lowering/step4/src_20000819-1.c/object-route.log`

Also reran the prepared dump:
`./build/c4cll --dump-prepared-bir --target riscv64-linux-gnu /workspaces/c4c/tests/c/external/gcc_torture/src/20000819-1.c`

Prepared dump result: exit code `0`, with output in:
`build/agent_state/575_rv64_pointer_arithmetic_lowering/step4/src_20000819-1.c/dump-prepared-bir.txt`
