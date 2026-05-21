Status: Active
Source Idea Path: ideas/open/376_aarch64_sizeof_loop_bound_stack_home_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Constant Loop-Bound Publication

# Current Packet

## Just Finished

Lifecycle switch complete. Umbrella inventory idea 295 is parked after
selecting focused owner idea 376 for the `00205` scalar constant/`sizeof`
loop-bound stack-home publication residual.

## Suggested Next

Execute Step 1: localize where the `sizeof`/constant-binary loop-bound values
for `00205` become unpublished stack homes before the AArch64 loop compares.

## Watchouts

Do not implement under idea 295. Keep this owner narrow to scalar constant or
`sizeof` loop-bound publication. Do not fold in `00187` external call-result
publication, `00174` scalar FP materialization, `00216` aggregate
initializer/relocation behavior, or timeout cases `00200` and `00207`.

## Proof

Lifecycle-only handoff. No implementation changes and no new test run.
