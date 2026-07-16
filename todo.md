Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.48
Current Step Title: Receive the one 859-authorized DirectScalar binary-fsub-LHS parameter authority row
你該做code review了
你該做test baseline review了

# Current Packet

## Just Finished

Completed `plan.md` Step 7.48 by receiving only the closed-859
`LirBinOp.scalar_lhs_parameter_authority` floating `fsub` LHS DirectScalar
parameter-use row into typed Raw BIR.

The receiver adds the bounded Raw-BIR `FSub` opcode and preserves the original
LHS parameter source value, owner, parameter index, lowered floating scalar
type, DirectScalar ABI, explicit LHS role, `fsub` opcode, LHS operand
identity, matching operation type, and a nonselected scalar RHS. LIR producer
authority remains owned by closed 859 and was not edited.

Focused receiver coverage verifies the positive Raw-BIR `direct_scalar_lhs`
payload and transactional rejection for omitted/missing, invalid, duplicate
definition, foreign owner, wrong index/type/ABI/role, non-`fsub`, LHS
mismatch, operation type mismatch, nonselected-RHS mismatch/type mismatch, and
duplicate selected consumer forms. Neighboring fadd/fmul LHS receiver
coverage now uses nonselected `fdiv` for nonmatching-opcode negatives.

## Suggested Next

Supervisor should accept the Step 7.48 receiver slice if the committed diff and
proof remain coherent, then ask plan-owner whether 734 should close, repair,
continue with another scoped handoff, or activate a successor. Do not start a
new row from this packet without lifecycle reassessment.

## Watchouts

Do not repeat Step 7.47, edit LIR producer authority, receive floating
binary-`fsub` RHS, other floating binary parameter uses, memory/VA,
aggregate/vector, module/type/global/metadata, residual
instruction/terminator, inline-assembly, ABI-expanded or aggregate
parameters, generic parameter sweeps, or any other family.

## Proof

Focused proof and broader backend LIR-to-BIR guard:

```
( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_interface$' ) > test_after.log 2>&1 && git diff --check
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed

( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir' ) > test_before.log 2>&1
( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir' ) > test_after.log 2>&1 && git diff --check
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Both matching before/after guards passed with no new or unresolved failures.
