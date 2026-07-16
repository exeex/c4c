Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.45
Current Step Title: Receive the one 856-authorized DirectScalar binary-fmul-RHS parameter authority row
你該做code review了
你該做test baseline review了

# Current Packet

## Just Finished

Completed `plan.md` Step 7.45 by receiving only the closed-856
`LirBinOp.scalar_rhs_parameter_authority` floating `fmul` RHS DirectScalar
parameter-use row into typed Raw BIR.

The receiver now preserves the original RHS parameter source value, owner,
parameter index, lowered floating scalar type, DirectScalar ABI, explicit RHS
role, `fmul` opcode, RHS operand identity, matching operation type, and a
nonselected scalar LHS. LIR producer authority remains owned by closed 856 and
was not edited.

Focused receiver coverage verifies the positive Raw-BIR `direct_scalar_rhs`
payload and transactional rejection for omitted/missing, invalid, duplicate
definition, foreign owner, wrong index/type/ABI/role, non-`fmul`, RHS mismatch,
operation type mismatch, nonselected-LHS mismatch/type mismatch, and duplicate
selected consumer forms.

## Suggested Next

Supervisor should accept the Step 7.45 receiver slice if the committed diff and
proof remain coherent, then ask plan-owner whether 734 should close, repair,
continue with another scoped handoff, or activate a successor. Do not start a
new row from this packet without lifecycle reassessment.

## Watchouts

Do not repeat Step 7.44, claim Raw-BIR receipt from 856, edit LIR producer
authority, receive another parameter row, or derive authority from text, names,
rendered operands, signatures, diagnostics, compatibility mirrors, `monostate`,
or testcase shape. Keep memory/VA, aggregate/vector, module/type/global,
residual instruction/terminator, inline-assembly, and other families fail
closed.

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
