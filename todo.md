Status: Active
Source Idea Path: ideas/open/380_rv64_object_route_short_circuit_call_argument_reload.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Short-Circuit Call Argument Loss

# Current Packet

## Just Finished

Lifecycle activation created the active runbook for Step 1.

## Suggested Next

Audit the short-circuit RHS call-argument loss for `src/20000112-1.c` using the
prepared call, argument, value-home, object-route, objdump, and qemu evidence
from idea 379. Record the first semantic preservation or reload shape in this
file before implementation.

## Watchouts

Do not key behavior on testcase name, `special_format`, a specific `strchr`
call, exact C expression spelling, block labels, value ids, instruction
indexes, physical registers, object addresses, or log text. Keep idea 379's
select/publication repair green and out of scope unless new evidence proves it
regressed.

## Proof

Lifecycle-only activation. No build proof required.
