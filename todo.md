Status: Active
Source Idea Path: ideas/open/360_lir_bir_vararg_va_arg_contract_completion.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Current Vararg Representation

# Current Packet

## Just Finished

Lifecycle activation created the runbook for Step 1. No implementation packet
has run yet.

## Suggested Next

Start Step 1 by auditing the frontend, LIR, BIR, and prepared representations
for variadic function identity and `va_*` operations. Use focused probes before
touching target object emission.

## Watchouts

Do not implement vararg semantics directly in RV64 object emission. Do not use
testcase names or GCC torture expectation changes as progress.

## Proof

No implementation proof required for lifecycle-only activation.
