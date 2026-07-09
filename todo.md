Status: Active
Source Idea Path: ideas/open/655_stack_destination_fan_in_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Prepare The Follow-Up Lifecycle Handoff

# Current Packet

## Just Finished

Completed Step 5: converted the selected authority-rejection seam into a
precise next executor handoff. The selected seam stays inside this
decomposition idea because it is a test/probe-only negative boundary for
missing stack-destination fan-in authority, not a new positive producer
implementation initiative.

## Suggested Next

to_subagent: c4c-executor

Objective: Add
`tests/backend/case/riscv64_stack_destination_authority_rejection.c` as a
negative focused probe for fail-closed stack-destination register fan-in
authority when no matching destination-authority producer fact exists.

Plan Step: Step 5 follow-up implementation packet for the selected
authority-rejection seam.

Owned Files: `tests/backend/case/riscv64_stack_destination_authority_rejection.c`,
`todo.md`, and only the narrow test harness metadata required for the repo's
normal backend case discovery if the new file is not auto-discovered.

Do Not Touch: `plan.md`, `ideas/open/**`, implementation source files,
expectations for unrelated tests, unsupported markers, allowlists, timeout or
pass/fail accounting files, root-level logs other than `test_after.log`, and
any idea 637 route files.

Proof Command Recommendation: run the narrow backend/prepared test command
that proves only
`tests/backend/case/riscv64_stack_destination_authority_rejection.c` and writes
the result to `test_after.log`. If the repo requires a compile/build step
before that test can run, run the matching narrow build first and include both
commands in `test_after.log`.

Done When: the new probe is present and validates that a two-register fan-in
into one stack destination records the visible source homes and destination,
but the bundle and each move remain `authority=none`, `parallel_copy=no`, with
fragment status
`producer_authority_missing_for_register_fan_in_stack_destination` when no
matching destination-authority producer exists. The packet is not done if it
passes by weakening expectations, marking the test unsupported, allowlisting
the failure, or adding producer implementation.

Watchouts: do not implement ordered final-state, mutual-exclusion, explicit
merge, or any other positive producer authority in this packet. Do not reopen
idea 637 or use `SelectMaterializationPreservedStackFallback`,
select-materialized preserved-stack fallback behavior, `%*.sel*` carriers,
source freshness, call-preservation, branch-load, select/join, diagnostic
wording, move order, or GCC torture testcase identity as destination-authority
proof. If adding this negative probe requires a broader lifecycle split,
implementation source changes, expectation downgrades, unsupported markers, or
allowlist changes, stop and report the blocker instead of editing those files.

## Watchouts

- The next packet is probe-only and negative by design; it must not become a
  producer implementation packet.
- Do not reopen idea 637 through `SelectMaterializationPreservedStackFallback`
  or select-materialized preserved-stack fallback behavior.
- Do not claim the rejected `src/20021204-1.c` route from unrelated `%t25`
  select facts or diagnostic wording.
- Do not treat `mutually-exclusive authority
  event_kind=before_instruction_copies` as positive evidence; the current
  excerpts still report `authority=none` and missing producer authority.
- Keep source freshness separate from destination authority; branch stack-load,
  call-preservation, source-publication, select/join, or carrier facts do not
  by themselves authorize multi-register fan-in into a stack destination.
- Ordered final-state, mutual-exclusion, and explicit merge remain blocked
  producer seams because no legal non-637 positive producer fact is currently
  proven.
- Do not make the probe pass through expectation downgrades, unsupported
  markers, allowlists, timeout/pass-fail accounting changes, or named-case
  shortcuts.

## Proof

No build or ctest proof required by the delegated packet because this was a
handoff-only `todo.md` update. Used the current `todo.md` Step 4 seam
selection and the active `plan.md`. Did not create or overwrite
`test_after.log`.
