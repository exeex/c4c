Status: Active
Source Idea Path: ideas/open/392_rv64_va_list_expression_call_argument_value_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture Post-Repair Runtime State

# Current Packet

## Just Finished

Lifecycle review after the continuation Step 4 found that idea 392 is still
not complete. Commit `9fb88adc` repaired the load-local payload rewrite and
the focused backend proof remains green, but the representative
`va-arg-13.c` still fails with `[RV64_BACKEND_RUNTIME_MISMATCH]`,
`clang_exit=0`, and `c4c_exit=Subprocess aborted`. The active runbook has been
replaced with a post-repair runtime/object-ABI tracing runbook.

## Suggested Next

Execute Step 1 from `plan.md`: regenerate fresh post-repair disassembly and
runtime evidence for `va-arg-13.c`, then record the caller argument object
contents and callee `dummy` entry state for both calls. Do not assume the old
`mv t1,s1` / local-storage-address overwrite remains; prove the current state
from fresh logs.

## Watchouts

- Do not hard-code `va-arg-13.c`, `test`, `dummy`, literal registers, stack
  offsets, or the abort branch.
- Keep the source `va_list` storage address, initialized save-area pointer
  payload, caller argument object address, caller argument object contents,
  callee parameter pointer, and callee `va_arg` loaded value separate.
- Do not reopen idea 391 unless fresh evidence shows incoming variadic GPR
  payloads are no longer saved into the backing area.
- If the remaining boundary is distinct from idea 392 publication/object-ABI
  requirements, stop and route it as a split instead of broadening this plan.

## Proof

Lifecycle-only rewrite. No build or runtime proof was run by the plan owner.
Relevant existing evidence remains in `test_after.log`,
`build/agent_state/392_cont_step4_va-arg-13.case.log`, and earlier
`build/agent_state/392_cont_step1_*` / `392_cont_step2_*` logs.
