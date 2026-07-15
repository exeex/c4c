# Current Packet

Status: Active
Source Idea Path: ideas/open/806_lir_phi_residual_producer_family_authority_trace.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Trace and classify the remaining 20060910-1.c producer seam

## Just Finished

806 Steps 1--2 are accepted: the postfix-increment old-value handoff was
repaired in `961ce9fda`; closed 807 and 808 separately resolved the floating
`fneg` and scalar bit-not `xor` residual families. The fresh full baseline
then passed 3036/3037, leaving only `llvm_gcc_c_torture_src_20060910_1_c`
with `LirPhiIncoming.value: must identify a known current-function LirValueId`.

## Suggested Next

Executor: trace only `llvm_gcc_c_torture_src_20060910_1_c` from the failing
PHI incoming to its immediate producer/lowering handoff. Record whether it
shares the accepted postfix route, one closed successor family, or an
unshared native producer family. Do not implement a repair in this packet.
After classification, plan-owner/supervisor must decide the smallest in-scope
repair packet or create a separately scoped successor before any full-baseline
retry.

## Watchouts

Do not reopen accepted postfix, floating `fneg`, scalar bit-not `xor`, or 804
unary-minus work. Do not sweep residuals, recover identity from text, or
treat the shared diagnostic as evidence of a shared producer. Do not treat a
partial baseline as parent clearance.

## Proof

Required current proof: trace evidence that identifies the immediate native
producer/lowering handoff and supports the classification. The subsequent
repair packet must carry fresh build plus nearby same-family positive and
malformed-authority proof; only then may the supervisor retry the 100% full
baseline gate.
