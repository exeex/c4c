Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.45
Current Step Title: Receive the one 856-authorized DirectScalar binary-fmul-RHS parameter authority row

# Current Packet

## Just Finished

Activated 734 after closed 856's accepted producer handoff. Steps 1 through
7.44 remain accepted historical work, most recently Step 7.44 receiver commit
`e0540da75`; 856's producer implementation is `a23031c8f` and lifecycle close
is `94ffc9d30`.

## Suggested Next

Execute `plan.md` Step 7.45: receive only the selected
`LirBinOp.scalar_rhs_parameter_authority` floating `fmul` RHS DirectScalar
parameter-use row into typed Raw BIR, importer dispatch, reachable verifier
path, and transactional positive/negative receiver coverage.

## Watchouts

Do not repeat Step 7.44, claim Raw-BIR receipt from 856, edit LIR producer
authority, receive another parameter row, or derive authority from text, names,
rendered operands, signatures, diagnostics, compatibility mirrors, `monostate`,
or testcase shape. Keep memory/VA, aggregate/vector, module/type/global,
residual instruction/terminator, inline-assembly, and other families fail
closed.

## Proof

Lifecycle-only activation. Required check: `git diff --check`.
