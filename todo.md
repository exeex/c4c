Status: Active
Source Idea Path: ideas/open/370_rv64_object_route_byval_aggregate_param_homes.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Byval Parameter-Home Facts

# Current Packet

## Just Finished

Idea 381 was accepted for closure and archived. Idea 370 is now active as the
next concrete open child repair under the RV64 gcc_torture prepared-shape
umbrella.

## Suggested Next

Execute Step 1 from `plan.md`: audit the prepared ABI/home facts for the
`src/20030914-2.c` byval aggregate parameter-home boundary and record the
smallest supportable shape or refined unsupported boundary.

## Watchouts

Do not infer aggregate layout or stack placement from source syntax or assumed
ABI state. Progress must consume explicit prepared parameter-home, frame, and
aggregate facts and keep unsupported byval shapes fail-closed.

## Proof

Lifecycle-only activation; no code proof required for the new Step 1 packet.

Close-time regression guard for idea 381 passed before this activation:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: before passed=13 failed=0 total=13; after passed=13 failed=0 total=13.
