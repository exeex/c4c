Status: Active
Source Idea Path: ideas/open/375_aarch64_local_aggregate_bitfield_layout_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Local Aggregate Bit-Field Layout Gap

# Current Packet

## Just Finished

Lifecycle switch completed after idea 374 closure. The local aggregate address
call publication owner is complete: `00218` and `00216` both advanced past
stale local-address call operands.

## Suggested Next

Execute Step 1 for idea 375. Start from `00218`: compare the source aggregate
and bit-field layout, prepared offsets, frame homes, generated store of
`AMBIG_CONV`, and `convert_like_real`'s `[x0,#16]` load/mask. Identify the
owning layout/store publication boundary before implementation.

## Watchouts

Do not reopen idea 374's local aggregate address call publication boundary.
`00216` should stay parked unless the `00218` localization proves the same
aggregate layout/store owner; its current visible residual is later
compound-relocation/function-pointer-table lowering.

## Proof

No new proof was run for idea 375 during activation. Idea 374 closure relied
on delegated focused proof plus the supervisor full-suite candidate that
matched the accepted baseline at `3352` passed / `23` failed / `3375` total
with no new failures.
