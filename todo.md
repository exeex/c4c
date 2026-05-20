Status: Active
Source Idea Path: ideas/open/348_aarch64_indexed_aggregate_address_writeback.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Indexed Aggregate Address Writeback Boundary

# Current Packet

## Just Finished

Lifecycle switch from umbrella inventory idea 295 to focused owner idea 348.

## Suggested Next

Execute Step 1 from `plan.md`: localize the first indexed aggregate
address/writeback boundary using the classified representatives `00130`,
`00176`, `00181`, `00182`, `00187`, and `00195`.

## Watchouts

- Keep implementation scoped to active focused idea 348, not the parked
  umbrella inventory idea.
- Do not special-case c-testsuite filenames, source symbols, byte offsets,
  temporary registers, or emitted instruction sequences.
- Keep boolean/comparison, FP expression, semantic admission,
  variadic/byval aggregate call-boundary, scalar cast, scalar local writeback,
  return publication, and direct-call publication work outside this route
  unless fresh first-bad-fact evidence proves the same owner.

## Proof

Lifecycle-only switch. No build or test run was performed.
