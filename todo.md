Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.42
Current Step Title: Receive DirectPointer Truthiness Parameter Authority

# Current Packet

## Just Finished

Activated 734 after closed 853's accepted DirectPointer pointer-truthiness
producer/schema/verifier handoff. Steps 1 through 7.41 remain accepted
historical receiver work and must not be repeated.

## Suggested Next

Implement only Step 7.42: receive the closed-853 tuple for an original
current-function DirectPointer parameter used by pointer truthiness lowering,
including parameter `LirValueId`, owner, parameter index, pointer type,
`DirectPointer` ABI, `PointerTruthiness` role, and the verified `PtrToInt`
plus `icmp ne i64 <ptr-int>, 0` consumer relation.

## Watchouts

Do not edit LIR producer/schema code, repeat accepted parameter rows through
fixed-direct-call argument 1, receive another parameter form, or recover
authority from pointer text, cast spelling, comparison text, signatures,
names, diagnostics, compatibility mirrors, or testcase shape.

## Proof

Passed matching focused regression proof:

`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_interface$' ) > test_before.log 2>&1`

`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_interface$' ) > test_after.log 2>&1 && git diff --check`

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: before 1/1, after 1/1, no new failures; `git diff --check` passed.
