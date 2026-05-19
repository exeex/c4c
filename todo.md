Status: Active
Source Idea Path: ideas/open/323_aarch64_vararg_consumption_source_progression.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Vararg Consumption Fault

# Current Packet

## Just Finished

Lifecycle reset for active idea 323 after closing idea 322.

## Suggested Next

Execute Step 1 from `plan.md`: localize the post-`va_start` vararg consumption
fault in generated `00204.c` output, mapping corrupt long-double/floating
loads or bad `va_list` progression back to prepared access plans, machine
records, and AArch64 variadic printer helpers.

## Watchouts

- The repaired `va_start` destination sequence from idea 322 must remain
  intact unless evidence proves it points at the wrong writable object.
- Raw `va.arg.aggregate*` text must remain absent.
- Do not special-case `00204.c`, `stdarg`, `myprintf`, one argument index, one
  type, one register, one stack slot, one offset, or one emitted instruction
  sequence.
- Do not weaken expectations, unsupported classifications, CTest registration,
  runner behavior, timeout policy, proof-log policy, or prior guardrails.
- Preserve the repairs from ideas 314, 315, 317, 318, 319, 320, 321, and 322.

## Proof

Close gate for idea 322 used the existing focused `test_before.log` and
`test_after.log` in non-decreasing mode: 10 passed, 1 failed, no new failures.
Step 1 should refresh `test_after.log` with the supervisor-delegated focused
proof command.
