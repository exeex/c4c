# Call Plan

Status: scaffold. Consumes the ABI plan and creates typed per-call input/output,
parallel-move, clobber, preservation, and return-recovery requirements. MIR call
lowering later realizes this plan.

Legacy coverage: `call_plans.*`, `regalloc/call_moves.*`, consumer moves,
call-return ABI, direct/indirect calls, tail-call eligibility, and call-boundary
publications.
