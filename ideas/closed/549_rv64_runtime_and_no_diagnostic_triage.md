# RV64 Runtime And No-Diagnostic Failure Triage

Status: Closed
Type: Evidence reconstruction and runtime triage
Parent: `ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md`
Owning Layer: Runtime mismatch, crash triage, or first-owner reconstruction

## Goal

Reproduce and classify current failures that lack explicit ownership evidence
before they are used to justify implementation work.

## Why This Exists

The current bucket map records 503 compile failures without an explicit
`unsupported_*` diagnostic, 57 other non-unsupported failures, and 7
segmentation-fault exits. These 567 rows are evidence gaps, not an
implementation-ready ordinary-C bucket.

## In Scope

- Reproduce representative no-diagnostic compile failures and crash rows.
- Identify whether rows belong to BIR producer, prepared contract, RV64
  lowering, runtime mismatch, test infrastructure, timeout, or unsupported
  feature owners.
- Split high-frequency reconstructed owners into durable follow-up ideas.
- Preserve minimal reproduction commands and logs for reviewer audit.

## Out Of Scope

- Claiming broad RV64 capability progress from unclassified failures.
- Implementing crash fixes without a reproduced first bad fact.
- Weakening runtime comparison, expected output, unsupported markers, or
  pass/fail accounting.
- Treating primary-F128 crashes as ordinary-C blockers.

## Acceptance Criteria

- Representative rows from each no-diagnostic family have reproducible logs.
- High-frequency first owners are named with current evidence or explicitly
  left as evidence gaps.
- Runtime mismatch work is separated from compile-time producer or lowering
  work.
- New implementation ideas are created only after first ownership is proven.

## Completion Notes

Closed after the active runbook completed all four evidence steps.

Evidence artifacts:

- Step 1 reproduction:
  `build/agent_state/549_step1_no_diagnostic_families/`
- Step 2 first-owner classification:
  `build/agent_state/549_step2_first_owner_classification/`
- Step 4 consolidation:
  `build/agent_state/549_step4_triage_consolidation/summary.md`

Durable follow-up ideas created:

- `ideas/open/569_prepared_move_bundle_ambiguous_multi_source_stack_destination.md`
  for the high-confidence prepared move-bundle classifier boundary reproduced
  by `src/20001026-1.c`.
- `ideas/open/570_rv64_unsupported_instruction_fragment_owner_diagnostics.md`
  for evidence-enabling diagnostics on the nine generic
  `unsupported_instruction_fragment` RV64 object-route rows.

Durable caveats:

- The exact standalone generated artifact behind the literal 503 compile-fail,
  57 other-fail, and 7 segfault counts remains missing/ambiguous. The triage
  preserves this as a provenance caveat and does not overclaim the 265-row
  family map found in `build/agent_state/546_step3_instruction_fragment_classification.tsv`.
- `src/20000910-1.c` remains quarantined with caveat: it was selected by the
  F128 family map, but its local source body is ordinary integer/pointer code
  while dumps include F128 declarations and sections from `stdlib.h`.
- `src/20030307-1.c` remains an explicit low-confidence evidence gap.
- No reproduced representative produced runtime mismatch, timeout,
  segmentation fault, or test-infrastructure failure.

Close gate:

- `cmake --build build`
- `ctest --test-dir build -j --output-on-failure -R backend > test_after.log`
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

## Reviewer Reject Signals

- Reject implementation claims based only on a crash, timeout, or compile-fail
  count without first-owner reproduction.
- Reject changing runtime comparison, expected output, or allowlists to turn
  failures into passes.
- Reject named-case fixes that make one reproduced row pass while nearby rows
  remain unclassified.
- Reject merging runtime mismatch, producer repair, and RV64 lowering into one
  broad slice.
- Reject leaving the same no-diagnostic failure hidden behind a new wrapper or
  diagnostic name.
