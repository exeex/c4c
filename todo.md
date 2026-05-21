# Current Packet

Status: Active
Source Idea Path: ideas/open/368_aarch64_00217_c_c_behavior_runtime_mismatch.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Post-Semantic Boundary

## Just Finished

Lifecycle activation created this active runbook from
`ideas/open/368_aarch64_00217_c_c_behavior_runtime_mismatch.md`.

## Suggested Next

Supervisor should select an executor packet for Step 1: reproduce and localize
the first incorrect post-semantic boundary for
`c_testsuite_aarch64_backend_src_00217_c`.

## Watchouts

- Do not reopen semantic `lir_to_bir` indirect local-memory admission unless
  the old semantic handoff failure returns.
- Do not special-case `00217`, `data`, exact output text, one cast spelling,
  one offset, one generated temporary, or one emitted instruction neighborhood.
- Do not edit expectations, unsupported classifications, runner behavior,
  timeout policy, CTest registration, proof-log policy, or external contracts.
- Keep idea 367 stability checks visible: `backend_lir_to_bir_notes`, `00005`,
  and `00173`.
- Existing untracked files under `review/` are transient artifacts and were not
  touched during activation.

## Proof

Lifecycle-only activation. No build or test run was required.
