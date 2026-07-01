Status: Active
Source Idea Path: ideas/open/506_rv64_out_of_ssa_phi_join_immediate_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Classify Generic Immediate Phi-Join Rows

# Current Packet

## Just Finished

Lifecycle activation created the active runbook and initialized execution
state for Step 1.

## Suggested Next

Execute Step 1 from `plan.md`: classify generic
`phi_join_immediate_materialization` rows from the 504 handoff evidence before
any implementation work.

## Watchouts

- Do not use select-publication evidence to justify generic immediate
  materialization.
- Do not infer constants from testcase names, source snippets, raw BIR shape,
  object output, or target register behavior.
- Keep missing immediate facts and ambiguous coordinates fail-closed.

## Proof

Lifecycle-only activation; no build or CTest proof required.
