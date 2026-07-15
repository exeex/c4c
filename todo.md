# Current Packet

Status: Active
Source Idea Path: ideas/open/806_lir_phi_residual_producer_family_authority_trace.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Repair the classified in-scope producer handoff and prove the blocker

## Just Finished

806 Step 3 is complete. `--dump-hir` for `20060910-1.c` traces `check_header`'s
ternary true expression to `*((deeper)->buffer_position)++`. Conditional
lowering preserves source authority when a PHI incoming already has it, and
the accepted 961 postfix path supplies the old pointer natively. Its enclosing
`UnaryOp::Deref`, however, emits a scalar `LirLoadOp` through a fresh temporary
string and loses the immediate `LirValueId` handoff. This is an in-scope,
unshared 806 producer family bounded to the scalar dereference-load result
handoff into the conditional PHI—not postfix, `fneg`, `xor`, CFG, or PHI
verifier work.

## Suggested Next

Executor: make the smallest native-authority repair to hand the scalar
`UnaryOp::Deref` load result into the conditional PHI, then add nearby
same-family positive and malformed-authority coverage. Run a fresh build and
focused proof. Do not retry the full baseline until that focused repair is
accepted.

## Watchouts

Do not reopen accepted postfix, floating `fneg`, scalar bit-not `xor`, or 804
unary-minus work. Do not change CFG, PHI verifier, or generic conditional
lowering behavior; do not recover identity from text or sweep residuals. A
focused green result is not parent clearance.

## Proof

Required current proof: fresh build plus nearby scalar-dereference
same-family positive and malformed-authority coverage, followed by focused
proof of `llvm_gcc_c_torture_src_20060910_1_c`. Only after supervisor accepts
that repair may it retry the 100% full-baseline gate.
