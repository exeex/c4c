Status: Active
Source Idea Path: ideas/open/673_post_664_full_suite_regression_probe.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce And Classify Regression Rows

# Current Packet

## Just Finished

Lifecycle review rejected closing idea 664 because the focused
`backend_riscv_object_emission` row now passes, but the supervisor-reported
full-suite baseline candidate regressed from the accepted 3386/3397 baseline
to 3384/3397 by adding rows 139 and 176.

Rows identified from the current CTest index:

- 139: `backend_cli_riscv64_pointer_global_local_publication`
- 176: `backend_obj_runtime_rv64_indirect_store_postincrement_callee_contract`

Idea 664 was parked with a durable note, and this new active plan was split to
keep those rows out of the row-256 object-emission source scope.

## Suggested Next

Delegate Step 1 to an executor: reproduce rows 139 and 176 with focused
commands, preserve `backend_riscv_object_emission` as a guard row, and write
classification evidence under `build/agent_state/673_step1_regression_probe/`.

## Watchouts

- Do not touch `review/reviewA.md`; it is a transient review artifact.
- Do not reopen row 256 unless `backend_riscv_object_emission` regresses.
- Do not change expectations, unsupported markers, allowlists, timeout policy,
  runtime policy, or baseline accounting.
- If rows 139 and 176 have different owners, split one into a separate source
  idea rather than coupling unrelated repairs.

## Proof

Lifecycle-only transition. No implementation validation was run by the plan
owner. Close for idea 664 was rejected because the broader baseline evidence
reported rows 139 and 176 as new full-suite failures despite the focused row
256 proof passing.
