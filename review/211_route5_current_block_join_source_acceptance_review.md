# Route 5 Current-Block Join-Source Acceptance Review

## Scope

- Active source idea: `ideas/open/211_route5_current_block_join_source_semantic_reader.md`
- Review question: whether the active diff moves exactly one current-block
  join-source semantic reader to Route 5/prepared agreement while preserving
  prepared fallback, branch/parallel-copy/output policy, joined-branch and
  prepared-output strings, and wrapper stability.
- Focus files:
  - `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
  - `tests/backend/mir/backend_aarch64_current_block_join_routing_test.cpp`
  - `tests/backend/bir/backend_x86_prepared_handoff_label_authority_test.cpp`
  - `todo.md`

## Review Base

- Chosen base commit: `d12e1f64b [plan] Activate Route 5 current-block join-source reader plan`
- Reason: this commit introduced the active `plan.md` for
  `ideas/open/211_route5_current_block_join_source_semantic_reader.md` and
  reset `todo.md` to Step 1. It is the activation checkpoint for the current
  source idea.
- Commits since base: 4

## Findings

No blocking findings.

Low: supervisor should recreate or preserve the acceptance proof log before
closing the plan. `todo.md` records a 7-test proof command and passing result,
but `test_after.log` was not present in the workspace during this review. The
configured CTest set includes the recorded targets
`backend_aarch64_current_block_join_routing`,
`backend_aarch64_instruction_dispatch`, `backend_prepared_lookup_helper`,
`backend_prepared_printer`, `backend_riscv_prepared_edge_publication`,
`backend_prepare_authoritative_join_ownership`, and
`backend_x86_prepared_handoff_label_authority`; it does not expose
`backend_x86_handoff_boundary`, matching the `todo.md` watchout. This is a
close-gate artifact issue, not a route-quality problem.

## Alignment Review

The implementation matches the source idea. The selected reader is
`current_block_join_prepared_query_source(...)`, reached through
`build_current_block_join_prepared_query_routing(...)`. The new agreement
helper checks Route 5 status, prepared-facts availability, prepared successor
label resolution, producer instruction identity, source-value identity,
predecessor label, destination value, and source value before accepting Route 5
as the source bit. See
`src/backend/mir/aarch64/codegen/dispatch_producers.cpp:158` and
`src/backend/mir/aarch64/codegen/dispatch_producers.cpp:186`.

Prepared fallback is retained. The routing builder still derives prepared join
facts before deciding source bits, and failed Route 5 agreement falls back to
`prepared_source` rather than claiming Route 5 authority. See
`src/backend/mir/aarch64/codegen/dispatch_producers.cpp:544` and
`src/backend/mir/aarch64/codegen/dispatch_producers.cpp:594`.

The diff stays limited to one semantic source reader. It does not migrate edge
publication, source-producer, move-bundle, branch policy, parallel-copy
scheduling, execution-site placement, final output, or wrappers into Route 5.
The x86 change is a test-only byte-stability freeze of the existing
contract-first wrapper stub, not a wrapper behavior change. See
`tests/backend/bir/backend_x86_prepared_handoff_label_authority_test.cpp:120`.

The fail-closed test matrix is appropriate for the named reader. The AArch64
current-block join routing test covers no prepared policy, positive prepared
agreement, missing predecessor prepared fallback, mismatched source, no source,
duplicate indexed Route 5 facts, memory-source rejection, and absent Route 5
identity. See
`tests/backend/mir/backend_aarch64_current_block_join_routing_test.cpp:427`.

I found no testcase-overfit signal. The production change is semantic
route/prepared agreement over Route 5 and prepared identities, not a named-case
string probe or expectation downgrade. The test changes add negative cases and
a byte-stable wrapper assertion; they do not weaken supported-path contracts or
refresh baselines to claim progress.

## Judgments

- Idea alignment: `matches source idea`
- Runbook transcription: `plan matches idea`
- Route alignment: `on track`
- Technical debt: `acceptable`
- Validation sufficiency: `narrow proof sufficient` for this slice, provided
  the supervisor recreates/preserves `test_after.log` before regression-guard
  handling and close.
- Reviewer recommendation: `continue current route`

## Suggested Supervisor Follow-Up

Recreate the recorded 7-test acceptance proof into `test_after.log`, run the
regression guard against `test_before.log`, and then hand the active state to
the plan owner for closure if the guard is monotonic.
