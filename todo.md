# Current Packet

Status: Active
Source Idea Path: ideas/open/796_lir_instruction_terminator_residual_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace and select the native cast-result authority family

## Just Finished

810 Step 3's isolated accepted-tree full gate built successfully but failed
3013/3037. Its 22 `LirCastOp.result` raw-text residuals are now routed to 796;
the 801 call and 806 PHI residuals remain outside this packet.

## Suggested Next

Trace representative `LirCastOp.result` failures to their native producer and
select exactly one cast-result authority family. Record any nonmatching case
before changing code.

## Watchouts

Do not touch the preserved dirty 801 aggregate/direct-complex hunks, 810/795
accepted GEP/parameter work, or the 806 PHI route. Do not parse `%t` text or
weaken the cast verifier.

## Proof

Start with a fresh build and focused representative cast-result reproductions.
Only after 796's bounded route is accepted may control return to 810 Step 3's
exact comparable full command.
