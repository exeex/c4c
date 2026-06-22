Status: Active
Source Idea Path: ideas/open/306_rv64_c_testsuite_undefined_main_triage_and_followups.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Close Triage Readiness Check

# Current Packet

## Just Finished

Step 5, Create Focused Follow-Up Ideas, created three focused open ideas:
`ideas/open/307_rv64_text_only_fail_closed_output_contract.md`,
`ideas/open/308_rv64_string_literals_and_extern_calls.md`, and
`ideas/open/309_rv64_aggregate_global_storage.md`. The first idea owns the
shared `rv64_text_only_fail_closed` output-contract cleanup and uses
`src/00094.c` as the minimal no-storage control. The secondary ideas split the
large `string_literals_and_extern_calls` bucket and the
`aggregate_global_storage` bucket into separate implementation routes with
candidate cases, acceptance criteria, and reviewer reject signals.

## Suggested Next

Begin Step 6 close-readiness check. Verify the classification summary,
representative evidence, repair order, and new follow-up ideas satisfy the
source idea acceptance criteria before deciding whether to close the active
triage plan.

## Watchouts

- Step 5 met the minimum follow-up count and included the required
  `rv64_text_only_fail_closed` idea plus two major secondary buckets.
- Step 6 should decide whether the remaining smaller buckets
  (`pointer_global_storage`, `floating_global_storage`,
  `scalar_global_storage`, and `aggregate_or_control_only_shape`) are adequately
  deferred by the repair-order artifact or need additional idea files before
  close.
- Do not register the full 220-case sweep as required CTest coverage.
- Do not treat empty `.text` output as a valid successful unsupported path.
- Keep root-level scratch logs clean.

## Proof

Lifecycle-only Step 5 update. No code build was required.

Ran:

```sh
test -f ideas/open/307_rv64_text_only_fail_closed_output_contract.md &&
test -f ideas/open/308_rv64_string_literals_and_extern_calls.md &&
test -f ideas/open/309_rv64_aggregate_global_storage.md &&
rg -n '## Candidate Cases|## Acceptance Criteria|## Reviewer Reject Signals|src/00094.c|rv64_text_only_fail_closed|string_literals_and_extern_calls|aggregate_global_storage|Current Step ID: 6|Current Step Title: Close Triage Readiness Check' \
  ideas/open/307_rv64_text_only_fail_closed_output_contract.md \
  ideas/open/308_rv64_string_literals_and_extern_calls.md \
  ideas/open/309_rv64_aggregate_global_storage.md \
  todo.md
```

Result: passed.
