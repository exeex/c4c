Status: Active
Source Idea Path: ideas/open/379_aarch64_local_aggregate_copy_load_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize First Bad Fact

# Current Packet

## Just Finished

Lifecycle handoff from umbrella inventory idea 295 to focused implementation
idea 379 is complete. The active runbook now owns the current non-timeout
`00216` `foo` local aggregate copy/load-local-memory publication crash.

## Suggested Next

Delegate Step 1 to localize the first bad fact before implementation: map the
current generated `foo` `ldrb w9, [x10]` crash back to the exact aggregate
copy, pointer-derived source, prepared value, MIR handoff, or AArch64
frame-slot publication boundary that fed the uninitialized stack-slot-derived
pointer.

## Watchouts

Do not implement under umbrella idea 295. Keep `00200` and `00207` parked as
timeout-only buckets. Keep later `00216` relocation/function-pointer-table
audit parked until the current `foo` crash advances and fresh first-bad-fact
evidence justifies a handoff. Do not special-case `00216`, `foo`, one local
variable, one frame offset, one register, or the exact emitted `ldrb`
neighborhood.

## Proof

Lifecycle-only switch; no build or test proof was run.
