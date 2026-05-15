Status: Active
Source Idea Path: ideas/open/241_f128_full_width_constant_carriers.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Prove The Dependency And Guard Adjacent Routes

# Current Packet

## Just Finished

Step 5 F128 full-width constant-carrier dependency proof is complete. Semantic
BIR F128 immediate call arguments now produce a prepared rematerialized
constant move into the existing AArch64 call-boundary consumer, with the
selected record carrying the structured low/high `constant_payload` through the
prepared source value id. Prepared printer and dispatch tests guard the adjacent
scalar, i128, helper, and printer routes without adding constant assembly
printing support.

## Suggested Next

Return to the supervisor for lifecycle review. If the source idea remains open,
the next packet should be plan-owner scoped rather than more Step 5 execution.

## Watchouts

- F128 immediate carriers still remain `kind=missing`; that is valid only for
  complete structured constant payloads tied to the prepared source value id.
- No AArch64 constant materialization printing was added. The proof stops at
  selected structured records and prepared/debug printer facts.
- F32/F64 scalar routes remain on scalar FPR banks; only F128 prepared
  call-boundary argument facts are promoted to the V-register/Q-register route.

## Proof

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log`

Result: passed, 139/139 backend tests. Proof log: `test_after.log`.
