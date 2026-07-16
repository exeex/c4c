Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.52
Current Step Title: Receive selected direct-local LirVaStartOp destination va_list authority

# Current Packet

## Just Finished

Lifecycle activated idea 734 from the accepted 867 handoff for one bounded
receiver row. No implementation has started in this runbook.

## Suggested Next

Execute `plan.md` Step 7.52 only: receive selected direct-local
`LirVaStartOp` destination `va_list` authority into typed Raw BIR using the
structured tuple documented in
`docs/lir_memory_va_object_lifetime_authority/handoff_to_734.md`.

## Watchouts

Do not repeat accepted idea 734 Steps 1 through 7.51. Do not edit LIR producer
authority or receive `va_end`, `va_copy`, `va_arg`, memcpy, memset, local/VLA,
prepared-BIR helper-home, target backend, aggregate/vector, parameter,
module/type/global, CFG/PHI, instruction/terminator, inline-assembly, or
generic residual rows. Never recover authority from spelling, printer output,
LLVM text, intrinsic names, rendered names, compatibility mirrors, or testcase
identity.

## Proof

Required for the implementation packet: fresh build, focused backend receiver
proof selected by the supervisor/executor, `git diff --check`, and broader
backend proof if shared Raw-BIR container, verifier, or importer helpers are
touched.
