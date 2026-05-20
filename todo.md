Status: Active
Source Idea Path: ideas/open/328_aarch64_byval_aggregate_call_argument_lane_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Repair Partial Upper-Lane Byval Publication

# Current Packet

## Just Finished

Lifecycle switch reactivated idea 328 after idea 331 classified the current
`00204.c` first bad fact as caller-side split aggregate byval lane
materialization/publication, not a `myprintf` stdarg cursor or copy owner.

## Suggested Next

Execute plan Step 1.

## Watchouts

Keep the repair semantic for AArch64 byval aggregate lane publication. Do not
key it to `00204.c`, `%9s`, `stdarg`, `myprintf`, `s9`, one stack slot, or one
emitted instruction sequence.

## Proof

No implementation proof has been run for the reactivated plan yet.
