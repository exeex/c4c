Status: Active
Source Idea Path: ideas/open/236_phase_e3_prepared_compare_join_selected_value_chain_metadata_follow_up.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate The Selected-Value-Chain Metadata Gap

# Current Packet

## Just Finished

Lifecycle switched from idea 235 to idea 236 after commit `ea2b3a133`
repaired Route 6 named scalar i32 call-argument source publication. The
delegated proof advanced past the idea 235 Route 6 assertion and now fails at
the split-out idea 236 selected-value-chain assertion.

## Suggested Next

Execute Step 1 of `plan.md`: reproduce or inspect the pointer-backed
same-module global selected-value-chain failure, identify the minimal prepared
compare-join metadata owner, and record the proposed Step 2 owner and proof
command here.

## Watchouts

- Preserve the idea 234 x86 compare-join stack-home repair as a regression
  guard.
- Preserve the idea 235 Route 6 consumed scalar i32 call-argument source repair
  as a regression guard.
- Do not change implementation files, expected strings, baselines, wrappers,
  helper-oracle output, Route 6 behavior, or x86 stack-home behavior during
  Step 1 unless the supervisor explicitly delegates implementation work.

## Proof

No code validation was run for this lifecycle-only switch. The prior delegated
proof for idea 235 failed only after advancing to the idea 236 assertion:

```text
scalar-control-flow compare-against-zero prepared compare-join pointer-backed same-module global selected-value chain return context ownership:
shared helper stopped publishing the true global-root selected-value chain
```
