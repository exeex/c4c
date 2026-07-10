# Post-Wave Residual Baseline Failures

Status: Open
Type: Umbrella triage and repair queue generator
Parent: `ideas/closed/658_backend_baseline_history_umbrella_triage.md`
Related:
- `test_baseline.log`
- `test_baseline.new.log`
- `log/baseline_5fef23bfa8b4eaf7f4cd2b897c25ff074d35209e.log`
- `log/baseline_0e049a6a270abb6999a4d1b6031fe70826ab5bd6.log`
- `ideas/closed/658_backend_baseline_history_umbrella_triage.md`
- `ideas/closed/659_rv64_byval_prepared_call_boundary.md`
- `ideas/closed/664_riscv_object_emission_internal_probe.md`
- `ideas/closed/668_llvm_torture_20040709_research.md`
- `ideas/closed/669_byval_prepared_dump_contract_review.md`
- `ideas/closed/670_byval_frame_slot_object_runtime_binaryinst.md`
- `ideas/closed/671_prepared_destination_dump_contract_review.md`
- `ideas/closed/672_stack_passed_parameter_home_dump_contract_split.md`
Owning Layer: post-wave baseline residual triage across newly exposed RV64 CLI
route rows and persistent dump/LLVM rows
Queue Order: 75
Proof Surface: `test_baseline.new.log`, which matches
`log/baseline_5fef23bfa8b4eaf7f4cd2b897c25ff074d35209e.log`
with `9/3397` failures.

## Goal

Classify and repair the remaining failures after the 658 follow-up wave, while
rejecting `test_baseline.new.log` as an accepted baseline until the newly
exposed RV64 CLI route rows are understood.

## Why This Exists

The 658 follow-up wave substantially reduced the broad baseline from the
original 34-failure surface to a 9-failure candidate, but the candidate is not
monotonic against the currently accepted `test_baseline.log`. It resolves
several accepted-baseline failures, including byval dump rows, RISC-V object
emission, and AArch64 instruction dispatch, but introduces or exposes two RV64
CLI route rows:

- `backend_cli_failure_riscv64_pointer_global_local_publication_live_load_rejection`
- `backend_cli_riscv64_call_arg_local_frame_address_materialization`

Those rows must not be accepted silently as baseline churn. The remaining
common failures also need a fresh post-wave classification because several
earlier ideas closed with partial or research-only outcomes.

## Current Evidence

- `test_baseline.log` from 2026-07-10 07:53 reports `11/3397` failures.
- `test_baseline.new.log` from 2026-07-10 12:25 reports `9/3397` failures.
- `log/baseline_5fef23bfa8b4eaf7f4cd2b897c25ff074d35209e.log` matches the
  9-failure candidate.
- New-only candidate failures versus accepted baseline:
  - `backend_cli_failure_riscv64_pointer_global_local_publication_live_load_rejection`
  - `backend_cli_riscv64_call_arg_local_frame_address_materialization`
- Resolved versus accepted baseline:
  - `backend_dump_riscv64_byval_aggregate_fixed_call`
  - `backend_dump_riscv64_byval_preserved_pointer_args`
  - `backend_riscv_object_emission`
  - `backend_aarch64_instruction_dispatch`
- Persistent common failures:
  - `backend_dump_riscv64_stack_passed_parameter_home_publication`
  - `backend_dump_riscv64_scalar_compare_frame_slot_destination`
  - `backend_dump_riscv64_prepared_fused_compare_call_result_predicate`
  - `backend_dump_riscv64_function_pointer_return_chain`
  - `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`
  - `llvm_gcc_c_torture_src_20040709_2_c`
  - `llvm_gcc_c_torture_src_20040709_3_c`

## In Scope

- Keep `test_baseline.new.log` as diagnostic evidence rather than accepting it
  until the new-only failures are classified or repaired.
- Compare accepted baseline, candidate baseline, and the latest
  `log/baseline_*.log` history by test name rather than numeric row id.
- Classify the two new-only RV64 CLI route rows by first owning layer.
- Reconcile persistent dump/CLI/LLVM failures with the closure notes from
  ideas 668, 671, and 672.
- Generate or activate focused follow-up implementation ideas only when a
  single owning layer is proven.

## Out Of Scope

- Accepting `test_baseline.new.log` while it contains failures not present in
  the accepted baseline.
- Reopening completed byval, RISC-V object-emission, or AArch64 ideas just
  because their old rows improved.
- Test expectation rewrites, unsupported-marker changes, allowlist edits,
  timeout changes, runtime policy changes, or baseline accounting changes.
- Mixing the new RV64 CLI route rows with persistent dump-contract or LLVM
  torture rows without evidence of a shared first owner.

## Acceptance Criteria

- The two new-only candidate failures are either repaired or assigned to a
  focused follow-up idea with a proven first owner.
- Persistent common failures are mapped to existing closure outcomes or new
  focused follow-ups without duplicating closed work.
- A fresh baseline candidate is either monotonic against the accepted baseline
  or explicitly rejected with preserved diagnostic evidence.
- No implementation progress is claimed through baseline acceptance,
  expectation churn, unsupported-marker changes, allowlist filtering, or
  weaker runtime checks.

## Reviewer Reject Signals

- Reject accepting `test_baseline.new.log` as a baseline while it still has
  new-only failures relative to `test_baseline.log`.
- Reject any route that keys off numeric row ids instead of stable test names.
- Reject merging RV64 CLI route failures, stale dump snippets, AArch64
  publication, and LLVM torture research into one implementation owner.
- Reject reopening closed ideas 659, 664, 669, 670, 671, or 672 without fresh
  evidence that their closure notes are wrong.
- Reject testcase-shaped shortcuts, expectation rewrites, unsupported
  downgrades, allowlist filtering, or weaker runtime checks as progress.

## Step 2 Split Outcome

Fresh Step 1 evidence is preserved in
`build/agent_state/675_step1_candidate_delta/summary.md`. That evidence shows
the two new-only RV64 CLI failures do not share one implementation owner:

- `backend_cli_failure_riscv64_pointer_global_local_publication_live_load_rejection`
  now reaches object emission and fails because the expected-failure wrapper
  reports an unexpected success. Its next owner is runtime/semantic object
  proof followed by test-contract or baseline routing if the emitted object is
  semantically valid.
- `backend_cli_riscv64_call_arg_local_frame_address_materialization` remains an
  RV64 object-route consumption issue for
  `LocalFrameAddressMaterialization`: text emission uses direct
  `addi a0, sp, offset`, while object emission materializes through an
  intermediate saved register and then copies to the ABI argument register.

The 675 active runbook is therefore retired at Step 2 instead of bundling both
rows into one implementation packet. Follow-up ideas:

- `ideas/open/676_rv64_pointer_global_local_publication_runtime_contract.md`
- `ideas/open/677_rv64_call_arg_local_frame_address_object_materialization.md`

Resume this umbrella only after those follow-ups settle the two new-only rows;
then continue with persistent common-failure reconciliation and a fresh
monotonic baseline candidate. Do not accept `test_baseline.new.log` from this
evidence point.

## Post-Follow-Up Resume Notes

Ideas 676 and 677 are now closed.

- 676 closed the pointer/global-local publication expected-fail route through
  runtime proof and a positive object contract.
- 677 repaired the RV64 object-route
  `LocalFrameAddressMaterialization` call-argument consumer. Focused proof
  passed for the CLI, route, dump, and `backend_riscv_object_emission` coverage
  named in the 677 closure notes.

Supervisor broad backend validation
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
now leaves only
`backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`
failing in backend scope. Resume 675 by reconciling that remaining backend row
against the persistent common-failure list and then decide whether it needs a
focused follow-up idea or can be resolved within the umbrella triage route.

`test_baseline.new.log` remains unaccepted until all remaining
candidate-only/common residual policy is settled and a fresh candidate is
monotonic against `test_baseline.log` or explicitly rejected with preserved
diagnostic evidence.
