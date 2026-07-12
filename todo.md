# Current Packet

Status: Active
Source Idea Path: ideas/open/716_prepared_call_plan_cursor_complete_production.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Produce one cursor-exact plan per supported call

## Just Finished

- Diagnosed plan Step 2's two remaining proof failures against the immediate
  pre-producer baseline `77bd1612b`. The joined-branch test, isolated at that
  baseline so earlier direct-call failures could not mask it, emits the exact
  same `register-source shared-publication edge moves` failure.
- The block-entry assertion and its complete implementation path are byte-for-
  byte unchanged from `77bd1612b` through `HEAD`; only call-boundary/view,
  call-plan, and direct-call fixture files changed. The producer repair merely
  made the executable advance far enough to expose this latent failure.
- No call-producer-owned causal defect remains, so no implementation or test
  change was made.

## Suggested Next

- Hand the prepared block-entry identity and joined-branch edge-publication
  failures to their owning initiative, or accept them as documented baseline
  blockers when deciding Step 2 completion.

## Watchouts

- The joined-branch module contains no calls, and the baseline-isolated runtime
  failure predates commits `80a5388a8` and `0b44f508e`.
- The block-entry failure uses manually assembled publication data and common
  MIR lookup code; neither its test section nor `prepared_lookups.cpp` or
  `mir/query.cpp` changed across the producer commits.

## Proof

- Ran the exact delegated command: `cmake --build --preset default && ctest
  --test-dir build -j --output-on-failure -R
  '^(backend_prepare_frame_stack_call_contract|backend_x86_handoff_boundary)$'
  | tee test_after.log`. Build succeeded; both unrelated failures remain, with
  complete output in `test_after.log`.
- In a detached `77bd1612b` worktree, built both proof targets and isolated the
  joined-branch runner; it failed with the identical edge-publication message.
  `git diff 77bd1612b..HEAD` confirms the block-entry test and lookup path are
  unchanged.
