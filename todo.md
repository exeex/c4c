Status: Active
Source Idea Path: ideas/open/235_phase_e3_route6_consumed_scalar_i32_call_argument_source_follow_up.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate The Route 6 Consumed-Plan Gap

# Current Packet

## Just Finished

Lifecycle switch complete: idea 234 is parked after the committed stack-home
isolation, and idea 235 is now active for the next implementation packet.

## Suggested Next

Execute plan Step 1. Reproduce or inspect the Route 6 consumed scalar i32
call-use boundary failure, identify the minimal consumed-plan lookup owner,
and record the Step 2 implementation surface plus proof command here before
editing implementation.

## Watchouts

- Keep this packet scoped to Route 6 named scalar i32 call-argument source
  facts in `ConsumedPlans`.
- Do not change x86 compare-join stack-home behavior from idea 234.
- Do not change prepared compare-join selected-value-chain metadata from idea
  236.
- Preserve Route 6 fail-closed behavior when facts are absent, prepared ids
  disagree, source names are missing, encodings are unsupported, or arguments
  are outside the named scalar i32 path.
- Do not rewrite route-debug expected strings, baselines, wrappers,
  helper-oracle output, or prepared call/debug output as proof.

## Proof

No code proof was run for this lifecycle-only switch.
