Status: Active
Source Idea Path: ideas/open/859_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select and publish one next body-parameter authority row
你該做code review了
你該做test baseline review了

# Current Packet

## Just Finished

Completed Step 1 by selecting and publishing exactly one next structured
body-parameter authority row: `LirBinOp.scalar_lhs_parameter_authority` for a
current-function `DirectScalar` floating parameter used as the LHS of binary
floating subtract `fsub`, with producer shape equivalent to `return x - 2.0;`.

The existing LIR emitter already populated the native parameter tuple. The
verifier now admits only this additional selected floating LHS consumer,
preserves the parameter value, owner, parameter index, typed `LirTypeRef`,
DirectScalar ABI, explicit LHS role, `fsub` opcode, LHS operand identity,
operation type, and nonselected scalar RHS, and rejects duplicate selected
floating-`fsub` LHS consumers.

Focused producer coverage verifies the positive row and transactional
malformed cases for omitted/missing authority, invalid value, duplicate
definition, foreign owner, wrong index/type/ABI/role, non-`fsub`, LHS
mismatch, operation type mismatch, selected-RHS incoherence, and duplicate
selected consumer forms. Neighboring fadd/fmul LHS negatives now use
nonselected `fdiv`.

## Suggested Next

Supervisor should accept the Step 1 producer handoff if the committed diff and
proof remain coherent, then ask plan-owner whether 859 should close and return
to 734 for the matching Raw-BIR receiver packet. Do not start Raw-BIR receiver
work from this packet before lifecycle closure.

## Watchouts

Do not edit Raw-BIR receiver code, reopen accepted DirectPointer or
DirectScalar receipts, select multiple rows, recover authority from
presentation text, or absorb memory/VA, aggregate/vector,
module/type/global/metadata, residual instruction/terminator, inline-assembly,
ABI-expanded or aggregate parameters, or target-lowering work.

## Proof

Focused proof and broader shared-LIR guard:

```
( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_function_signature_type_ref$' ) > test_after.log 2>&1 && git diff --check
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed

( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_' ) > test_before.log 2>&1
( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_' ) > test_after.log 2>&1 && git diff --check
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Both matching before/after guards passed with no new or unresolved failures.
