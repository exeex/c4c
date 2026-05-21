Status: Active
Source Idea Path: ideas/open/365_aarch64_signed_remainder_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Signed Remainder Operand Flow

# Current Packet

## Just Finished

Lifecycle switch only. Idea 364 is scope-satisfied but close-deferred: the
duplicate `.Lselect_mat_*` owner is repaired and `00143.c.s` now has 152
synthetic select label definitions with zero duplicates, but the strict
close-time regression guard rejected archival closure because the matching
focused scope stayed at 143/144 passing.

The new first bad fact is signed remainder lowering in `00143`: `count % 8`
with `count == 39` should produce selector `7`, but generated AArch64 computes
`39 - 4 * 4 == 23` by using the quotient as both operands to `msub`. This is
outside idea 364 and outside the parked unsigned-only div/rem owner 350, so
idea 365 is now active.

## Suggested Next

Execute Step 1 from `plan.md`: inspect generated `00143.c.s` and the relevant
scalar div/rem lowering records to identify where the original divisor is
lost or replaced by the quotient before signed remainder `msub` emission.

## Watchouts

Do not reopen synthetic label uniqueness, unsigned div/rem, switch lowering,
or Duff's-device-specific behavior without fresh first-bad-fact evidence. The
owner is signed remainder operand flow.

## Proof

Ran matching close-time focused proof after the Step 4 classification:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_.*|c_testsuite_aarch64_backend_src_00143_c)$' > test_after.log 2>&1
```

Then ran the regression guard against the existing matching `test_before.log`:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log
```

Result: close rejected by strict guard. Both logs report 143 passed, 1 failed,
144 total, with no new failing tests, but the passed count did not strictly
increase.
