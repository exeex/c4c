Status: Active
Source Idea Path: ideas/open/328_aarch64_byval_aggregate_call_argument_lane_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Byval Aggregate Call-Argument Lane Gap

# Current Packet

## Just Finished

Lifecycle split after idea 327 Step 2 commit `de571342a`: fixed-formal entry
publication is parked, and caller-side byval aggregate call-argument
register-lane publication is now active as idea 328.

## Suggested Next

Execute Step 1 from `plan.md`: localize `arg` callsites for `fa_s1` and
`fa_s2`, mapping semantic byval arguments through prepared source storage into
the missing AAPCS64 register lanes before `bl`.

## Watchouts

Do not continue this as callee fixed-formal entry publication. The reviewer
accepted the idea 327 slice as progress and identified the next first bad fact
as caller-side byval aggregate call-argument publication.

## Proof

Lifecycle-only split. No code validation was run.
