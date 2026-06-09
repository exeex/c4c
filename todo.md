Status: Active
Source Idea Path: ideas/open/138_call_plan_lookup_ownership_cleanup.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Prove backend behavior and review ownership quality

# Current Packet

## Just Finished

Step 5 proved backend behavior and reviewed the ownership cleanup for drift.

- The diff remains an API ownership cleanup: call-plan lookup declarations now
  live in the call-owned boundary, definitions and aggregate construction stay
  in the prepared lookup implementation, and consumers reach the call boundary
  where they use call-plan lookup facts.
- No ABI classification, call lowering semantics, preservation route
  calculation, call-boundary effect construction, AArch64 register handling,
  test expectations, unsupported markers, or testcase-shaped shortcuts changed.
- AArch64 consumers still consume prepared call facts; no local scans or
  duplicated preservation, outgoing-stack, or call-boundary discovery logic
  were introduced.

## Suggested Next

Active runbook steps are complete. Ask the plan owner to decide whether to
close the linked source idea or replace/deactivate the plan.

## Watchouts

- Plan-owner close review should compare the final diff against
  `ideas/open/138_call_plan_lookup_ownership_cleanup.md` acceptance criteria.
- `src/backend/mir/aarch64/module/module.hpp` still includes the broad prepared
  lookup facade because `FunctionLoweringContext` owns broad prepared lookup
  pointers as well as the call-plan lookup pointer.

## Proof

`cmake --build --preset default` succeeded.
`ctest --test-dir build -j --output-on-failure -R '^backend_'` passed 179/179
tests into `test_after.log`.
Regression guard with `--allow-non-decreasing-passed` passed against
`test_before.log`: before 179/179, after 179/179, no new failures, no new
tests over 30 seconds. The fresh after log was rolled forward to
`test_before.log`.
