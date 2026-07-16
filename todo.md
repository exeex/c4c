Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.44
Current Step Title: Receive DirectScalar Binary-Fmul LHS Parameter Authority
你該做code review了

# Current Packet

## Just Finished

Closed 855 as the producer/verifier handoff for one selected
`LirBinOp.scalar_lhs_parameter_authority` row: a current-function
`DirectScalar` floating parameter used as the LHS of binary floating `fmul`.
734 is resumed after accepted Step 7.43 for only the matching typed Raw-BIR
receiver packet.

## Suggested Next

Execute `plan.md` Step 7.44. Receive only the closed 855 binary-`fmul` LHS
parameter authority row into typed Raw BIR with transactional positive and
malformed coverage.

## Watchouts

Do not repeat Step 7.43, claim Raw-BIR receipt from 855, receive another
parameter row, or recover authority from presentation fields. Later parameter,
memory/VA, aggregate/vector, module/type/global, instruction/terminator, and
inline-assembly forms remain out of scope.

## Proof

Run a fresh build plus focused backend receiver proof selected by the
supervisor, followed by `git diff --check`.
