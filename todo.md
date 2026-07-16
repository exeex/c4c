Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.51
Current Step Title: Receive the one 864-authorized double(double) direct call-result authority row

# Current Packet

## Just Finished

Closed 864 after accepted producer-side publication of exactly one next
non-body-parameter row:
`LirCallOp.direct_one_double_arg_scalar_floating_call_authority` for direct
nonvariadic `double(double)` call results. Reactivated 734 after accepted Step
7.50; Steps 1 through 7.50 remain historical progress and must not be repeated.

## Suggested Next

Execute `plan.md` Step 7.51 only: receive the closed-864 direct nonvariadic
`double(double)` `LirCallOp` result authority into typed Raw BIR, preserving
the handed-off result/owner/callee/`double(double)`/`DirectCallResult` tuple.

## Watchouts

Do not edit LIR producer authority, repeat Step 7.50, reopen fixed direct-call
arguments 0/1, receive any body-parameter row, require selected downstream
floating binary LHS consumer coherence, recover authority from presentation
text, or absorb memory/VA, aggregate/vector, module/type/global/metadata, CFG/
PHI, residual instruction/terminator, inline-assembly, generic residual
sweeps, or any other family.

## Proof

No proof has run for Step 7.51 yet. Expected proof starts with a fresh build,
focused backend LIR-to-BIR receiver test, `git diff --check`, and broader
backend proof if shared importer or verifier code is touched.
