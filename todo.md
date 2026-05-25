Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Consolidation Acceptance Checkpoint

# Current Packet

## Just Finished

Completed Step 5 of `plan.md`: assessed the remaining AArch64 calls surface
against `ideas/open/02_aarch64_calls_emission_consolidation.md` acceptance
criteria and ran the supervisor-selected full-suite regression guard proof.

Acceptance assessment:

- The source idea is not closure-ready yet.
- Acceptance criteria are partially satisfied: `calls_preservation.cpp` is
  retired, removed from build metadata, and prepared call-boundary effect
  planning now owns preservation home population/republication ordering for
  the call-boundary loops.
- Remaining AArch64 calls surface still includes `calls_argument_sources.cpp`,
  `calls_byval_aggregates.cpp`, `calls_moves.cpp`, `calls_printing.cpp`,
  `calls_dispatch_bridge.cpp`, `calls_common.cpp`, `calls.cpp`, and
  `calls.hpp`.
- The remaining blocker is the same narrow authority gap from the source idea:
  caller-side selected `BeforeCall` argument source selection still spans
  prior preservation, local-frame address materialization, frame-slot/address
  sources, and byval register-lane materialization without a single prepared
  call-argument source fact that owns that choice.
- `calls_moves.cpp` now consumes prepared boundary effects directly for
  explicit moves and preservation effects, but the remaining source-selection
  helpers are not proven duplicate planning authority that can be deleted under
  this runbook without new shared prepared-source authority.

## Suggested Next

Supervisor should route a lifecycle decision rather than close this source idea
as complete. The narrow follow-up/blocker is to introduce or prove a shared
prepared call-argument source fact for selected `BeforeCall` argument moves, so
AArch64 lowering can consume the selected source instead of locally choosing
between prior preservation, local-frame address materialization, frame-slot
sources, and byval register-lane materialization.

## Watchouts

- Full-suite proof is monotonic, but it is not sufficient for closure because
  the source idea's semantic acceptance criterion about target-local rederived
  call-plan decisions is still blocked by the missing selected-source fact.
- `calls_argument_sources.cpp` and `calls_byval_aggregates.cpp` remain valid
  watch areas for the follow-up because they still provide the source/address
  and byval lane materialization used by `lower_before_call_move`.
- `calls_printing.cpp` remains a separate printer/diagnostic boundary after the
  generic machine-effect spelling move; no additional closure claim should be
  made solely from file count.
- No implementation files, tests, build metadata, plan files, source ideas, or
  review artifacts were touched in this packet.

## Proof

Ran delegated proof successfully:
`cmake --build --preset default && (ctest --test-dir build -j --output-on-failure > test_after.log 2>&1 || true) && python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: build was up to date; full CTest completed with the existing baseline
failure set; regression guard passed with non-decreasing passed tests.

Regression guard summary:

- before: passed=3400 failed=10 total=3410
- after: passed=3400 failed=10 total=3410
- delta: passed=0 failed=0
- new failing tests: 0
- result: PASS

Proof log: `test_after.log`.
