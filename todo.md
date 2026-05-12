Status: Active
Source Idea Path: ideas/open/186_bir_direct_symbol_identity_validation_closure.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory direct-symbol validation surfaces

# Current Packet

## Just Finished

Lifecycle close/switch completed for idea 185. Idea 186 is now active and ready
for Step 1 inventory.

## Suggested Next

Begin Step 1 by auditing BIR direct-symbol validation surfaces and recording the
selected generated metadata-rich hardening target in this file.

## Watchouts

- Preserve runtime/intrinsic placeholder calls as explicit invalid-id paths;
  do not conflate them with user/extern symbol identity.
- Cover direct calls and at least one global or pointer-initializer symbol
  reference before recommending closure.
- The source idea dependency list still names idea 183 under `ideas/open/`;
  idea 183 is already closed and may need a separate metadata repair.

## Proof

Lifecycle-only switch. Close-time backend regression guard used existing
canonical logs:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: passed. Before and after both report 109 passed, 0 failed.
