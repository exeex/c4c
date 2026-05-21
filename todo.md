Status: Active
Source Idea Path: ideas/open/378_aarch64_scalar_fp_expression_constant_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize First Scalar FP Bad Fact

# Current Packet

## Just Finished

Lifecycle switched from umbrella inventory idea 295 to focused owner idea 378.
No implementation work has started under this plan.

## Suggested Next

Execute Step 1: localize the first scalar FP constant or expression value in
`00174` that is missing or stale before its generated AArch64 consumer.

## Watchouts

Do not special-case `00174`, one output row, one FP register, or one emitted
instruction sequence. Keep `00216`, `00200`, and `00207` parked unless fresh
evidence proves they share this scalar FP owner.

## Proof

Lifecycle-only switch. No build or runtime proof was run.
