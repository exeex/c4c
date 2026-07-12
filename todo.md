# Current Packet

Status: Complete
Source Idea Path: ideas/open/721_x86_defined_function_prepared_core_completion.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate The Broader X86/Backend Contract

## Just Finished

- Step 4 completed closure validation for the prepared-core producer repair.
  The focused Step 3 build and
  `^backend_prepare_frame_stack_call_contract$` proof passed, and the canonical
  matching full-suite comparison passed monotonically: both commit `8d7ea2da5`
  and commit `792f7db3b` passed 3374/3429 with the same 55 known failures, zero
  new failures, and zero new tests over 30 seconds. The earlier accepted
  baseline improved the source failure count from 56 to 55.
- Acceptance criterion 1 confirmed: Step 1 documented the failing defined
  function as `grouped_spill_reload_contract`, stable `FunctionNameId` 0, and
  the earliest missing fact as absent prepared control flow when later common
  preallocation phases were entered without legalization.
- Acceptance criterion 2 confirmed: focused coverage proves both the repaired
  manual-phase grouped-spill definition and an ordinary fully legalized
  definition receive genuine common `PreparedMirCoreView` views with stable
  BIR, control-flow, addressing, and value-location identity.
- Acceptance criterion 3 confirmed: missing addressing and mismatched
  value-location banks remain rejected at common prepared-core admission; the
  x86 emitter invariant remains fail closed and unchanged.
- Acceptance criterion 4 confirmed: focused proof and matching full-suite proof
  are green without expectation downgrade or fixture-only readiness injection.
- Reviewer reject signals checked explicitly: the committed repair contains no
  fixture-name, function-name, or single-test conditional; it does not remove,
  weaken, catch, or bypass the x86 invariant; it does not mark the fixture
  unsupported, rewrite expectations, treat a definition as a declaration, or
  inject readiness in the fixture; it claims no helper rename or status rewrite
  as capability progress; it changes no idea 718 attribution, idea 716 cursor
  semantics, ABI policy, or unrelated x86 emission; and the original missing
  prepared-core failure is resolved at the earliest general preallocation phase
  boundary rather than hidden behind a later abstraction.

## Suggested Next

- Supervisor: hand this complete, closure-ready runbook packet to the plan owner
  for the lifecycle closure decision.

## Watchouts

- None. All source-idea acceptance criteria and reject signals are resolved.

## Proof

- Focused proof passed in Step 3: build plus
  `ctest --test-dir build -j --output-on-failure -R '^backend_prepare_frame_stack_call_contract$'`
  (1/1).
- Ran exactly:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`.
- Regression guard passed: before 3374 passed / 55 failed / 3429 total; after
  3374 passed / 55 failed / 3429 total; zero resolved or new failures and zero
  new tests over 30 seconds. Canonical matching full-suite logs:
  `test_before.log` and `test_after.log`.
