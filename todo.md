Status: Active
Source Idea Path: ideas/open/310_rv64_c_testsuite_qemu_nonzero_runtime_triage.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Final Triage Check

# Current Packet

## Just Finished

Step 7: Final Triage Check verified the active RV64 runtime triage runbook
against the requested closure checklist.

Final checklist result: ready for lifecycle closure review by plan-owner.

- Latest probe provenance is recorded as
  `build/rv64_c_testsuite_probe_latest/summary.tsv`.
- Real source rows in the latest 220-case probe, excluding the two allowlist
  comment rows: `PASS=67`, `EMIT_FAIL=26`, `CLANG_FAIL=4`,
  `QEMU_NONZERO=122`, `QEMU_TIMEOUT=1`, `TOTAL=220`.
- Runtime failures are fully covered by the Step 4 classification:
  `QEMU_NONZERO 132=90`, `QEMU_NONZERO 139=31`, `QEMU_NONZERO 112=1`,
  `QEMU_TIMEOUT 124=1`, `TOTAL=123`.
- Semantic runtime buckets cover all 123 runtime rows:
  `unresolved_external_or_empty_stub_emission=59`,
  `incomplete_control_or_expression_emission=23`,
  `stack_pointer_local_slot_materialization=21`,
  `global_storage_or_global_address_flow=8`,
  `wide_or_narrow_scalar_storage_lowering=7`, and
  `unmaterialized_value_or_comparison_result=5`.
- Representative backend evidence exists in
  `build/rv64_c_testsuite_probe_latest/triage_step3/summary.md` and its
  per-case scratch dumps for `src/00005.c`, `src/00008.c`, `src/00112.c`, and
  `src/00025.c`.
- Illegal instruction, segmentation fault, non-crashing/non-132 nonzero, and
  timeout behavior are explicitly separated. The only non-132/non-139
  non-timeout row is `src/00112.c` with status `112`, classified as
  `unmaterialized_value_or_comparison_result`; no genuine intentional
  non-crashing nonzero-semantics bucket was found.
- Focused follow-up ideas exist for three repair initiatives:
  `ideas/open/311_rv64_ordinary_control_expression_completion.md`,
  `ideas/open/312_rv64_local_stack_slot_address_materialization.md`, and
  `ideas/open/313_rv64_external_empty_stub_policy_runtime.md`.
- Generated bulk artifacts checked for this run live under
  `build/rv64_c_testsuite_probe_latest/`; no matching generated `.tsv`, `.out`,
  `.s`, `.bin`, `.bir.txt`, or `.prepared-bir.txt` artifacts were found at repo
  root.

## Suggested Next

Supervisor should ask plan-owner to close or otherwise lifecycle-dispose the
active source idea
`ideas/open/310_rv64_c_testsuite_qemu_nonzero_runtime_triage.md`.

## Watchouts

No implementation repair was performed under this triage-only idea. The next
backend work should start from the focused follow-up ideas rather than expanding
this triage runbook. The external empty-stub family remains the largest raw
bucket, but it is a separate policy/runtime initiative from ordinary RV64
control/expression and local stack/address lowering.

## Proof

No build or CTest proof required for this triage-only final checklist. Proof was
read-only inspection of `plan.md` Step 7, `todo.md`,
`build/rv64_c_testsuite_probe_latest/summary.tsv`,
`build/rv64_c_testsuite_probe_latest/triage_step3/summary.md`,
`build/rv64_c_testsuite_probe_latest/triage_step4/summary.md`,
`build/rv64_c_testsuite_probe_latest/triage_step4/classification.tsv`, and
follow-up ideas 311, 312, and 313. No `test_after.log` was produced because the
delegated proof explicitly did not require build/ctest proof.
