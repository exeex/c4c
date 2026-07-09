Status: Active
Source Idea Path: ideas/open/616_select_publication_source_wiring.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Refresh Residuals And Split Decision

# Current Packet

## Just Finished

Step 3, `Refresh Residuals And Split Decision`, refreshed the named residual
rows after the Step 2 stack-source select-publication consumer.

- Fresh targeted probe path: `/tmp/c4c_616_step3_probe`.
- The seven original stack-offset select-publication rows no longer report
  `unsupported_source_stack_offset`.
- `src/20000706-1.c`, `src/20000706-2.c`, `src/20000717-5.c`,
  `src/20071213-1.c`, `src/20120427-1.c`, and `src/20120427-2.c` now reach
  `[RV64_BACKEND_RUNTIME_MISMATCH]`. Classification: downstream
  non-select-publication work.
- `src/991216-1.c` now fails first on generic
  `unsupported_move_bundle_target_shape` without a current select-publication
  rejection. Classification: downstream non-select-publication move-bundle
  target-shape work.
- The four source-home guard rows remain fail-closed:
  `src/pr45034.c`, `src/pr53160.c`, `src/pr58726.c`, and `src/pr59221.c` all
  report `selected_move_carrier=select_materialization`,
  `intent_status=unsupported_source_home`, and
  `select_publication_rejection_reason=intent_status_unsupported_source_home`.
  Classification: separate owner; they need a durable source-home/source
  freshness initiative, not more stack-offset wiring in this plan.
- The two visible large-immediate select-publication rows,
  `src/pr29695-1.c` and `src/pr29695-2.c`, both report
  `intent_status=available`, `intent_source_immediate_i32=2147483648`, and
  `select_publication_rejection_reason=unsupported_source_immediate_i32_range`.
  Classification: separate owner; this is large-immediate materialization/range
  work, not same-owner stack-offset publication wiring.
- `src/921124-1.c` and `src/920710-1.c` still fail first on
  `unsupported_terminator_fragment`. Classification: downstream
  non-select-publication terminator work.
- Step 4 close-readiness classification should proceed. The refreshed rows do
  not show remaining same-owner Step 2 stack-offset select-publication work;
  source-home and large-immediate residuals should be durable follow-up splits
  if the supervisor wants to pursue them.

## Suggested Next

Run Step 4, `Close-Readiness Classification`, and decide whether idea `616`
can close with the current source-freshness boundary proof plus follow-up
classification for source-home and large-immediate residuals.

## Watchouts

- Do not pull `src/pr45034.c`, `src/pr53160.c`, `src/pr58726.c`, or
  `src/pr59221.c` into this plan by treating `unsupported_source_home` as
  established source freshness.
- Do not pull `src/pr29695-1.c` or `src/pr29695-2.c` into this plan by
  widening Step 2's stack-source path into large-immediate range/materialization
  work.
- The six runtime-mismatch rows and the two terminator rows are not evidence
  for more select-publication source wiring; they need separate downstream
  ownership if pursued.
- No implementation files, unsupported markers, allowlists, expectations,
  runtime/accounting files, `plan.md`, or source ideas were changed.

## Proof

Diagnostics-only packet. No fresh backend proof was required and
`test_after.log` was not created or modified.

Targeted row probe:
manual runner calls derived from
`tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake`, writing
only under `/tmp/c4c_616_step3_probe`, refreshed the seven stack-offset rows,
four source-home guard rows, two visible large-immediate rows, and
`src/921124-1.c`/`src/920710-1.c`.
