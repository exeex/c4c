# Current Packet

Status: Active
Source Idea Path: ideas/open/52_aarch64_calls_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Establish variadic aggregate va_arg cursor baseline

## Just Finished

Lifecycle switched from the parked ALU prepared-authority route to the calls
prepared-authority route after ALU Step 7 classified the remaining focused
failure as calls/variadic ownership. No implementation packet has run under
this calls plan yet.

## Suggested Next

Execute Step 1: establish the `00204` variadic aggregate `va_arg` cursor
baseline, trace the prepared aggregate access plan, and identify whether the
bad overflow cursor is selected by prepared facts, record construction, or
printer/lowering emission.

## Watchouts

The first target is the `%9s` aggregate `va_arg` path in `myprintf` from
`c_testsuite_aarch64_backend_src_00204_c`. The last classification observed
`ldrb w9, [x13]` with `x13 = 0x10` after the overflow path advanced from a
spill slot instead of from the loaded overflow pointer.

Keep the ALU 5/6 focused subset as a guardrail. The ALU idea remains open and
should not be closed by this route switch.

## Proof

No calls-route proof has run yet after the lifecycle switch. The previous ALU
route proof passed the two ALU probes plus `00164`, `00176`, and `00181`, and
failed only `c_testsuite_aarch64_backend_src_00204_c`.
