Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.42
Current Step Title: Receive DirectPointer Truthiness Parameter Authority

# Current Packet

## Just Finished

Step 7.42 received the closed-853 DirectPointer pointer-truthiness
body-parameter authority row into typed Raw BIR. The accepted tuple is the
original current-function parameter `LirValueId{81}`, owner
`pointer_truthiness_parameter_owner`, parameter index `0`, pointer
`LirTypeRef`, `DirectPointer` ABI, `PointerTruthiness` role, and the verified
`PtrToInt` plus `icmp ne i64 <ptr-int>, 0` consumer relation. The focused
backend coverage now includes the positive Raw-BIR receipt and malformed
missing, invalid, foreign, duplicate, owner/index/type/ABI/role, non-PtrToInt,
operand, compare-LHS, zero-RHS, and duplicate-row rejection cases.

## Suggested Next

Ask plan-owner to reassess source completion for 734 now that Step 7.42 is
implemented and the runbook appears exhausted.

## Watchouts

No LIR producer/schema files were edited. Keep any further work out of adjacent
parameter rows unless plan-owner repairs or replaces the runbook.

## Proof

Passed delegated focused proof:

`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_interface$' ) > test_after.log 2>&1 && git diff --check`

Result: build passed, `backend_lir_to_bir_interface` passed 1/1, and
`git diff --check` passed. Proof log: `test_after.log`.
