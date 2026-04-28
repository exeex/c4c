# x86 Prepared Module Renderer Recovery

Status: Closed
Created: 2026-04-26
Closed: 2026-04-28

Related Ideas:
- [120_bir_raw_label_fallback_cleanup_after_assembler_id_path.md](/workspaces/c4c/ideas/open/120_bir_raw_label_fallback_cleanup_after_assembler_id_path.md)

## Goal

Recover or rebuild x86 prepared-module rendering capability when the project
wants x86 handoff/codegen tests to execute real scalar and control-flow asm
again.

## Why This Idea Exists

During Step 5 of idea 120, an attempted x86 handoff proof drifted into
shape-specific scalar asm rendering in `src/backend/mir/x86/module/module.cpp`.
The reviewer rejected that dirty slice in
`review/step5_x86_handoff_dirty_slice_review.md` as testcase-overfit and route
drift from raw-label fallback cleanup.

This work may still be valuable, but it is target renderer recovery. It should
not be silently absorbed into idea 120 or used as the means of proving
`BlockLabelId` handoff authority.

## Scope

- Inventory the current x86 prepared-module/codegen surfaces and missing test
  infrastructure.
- Restore compile compatibility for x86 backend handoff tests if the missing
  headers or replacement APIs are available.
- Restore stale x86 backend BIR/handoff tests so they can describe the real
  supported/unsupported renderer boundary instead of failing on harness drift.
- Design real renderer coverage for scalar and control-flow forms without
  instruction-count or testcase-shape shortcuts.
- Prove prepared control-flow label consumption through the x86 path only after
  renderer capability is restored on semantic grounds.

## Non-Goals

- Do not add narrow pattern dispatch that only recognizes current tests.
- Do not weaken supported-path tests or rewrite expectations to hide missing
  renderer capability.
- Do not treat x86 renderer recovery as acceptance progress for idea 120 unless
  the active plan is explicitly switched to this source idea.

## Acceptance Criteria

- x86 prepared-module rendering has a documented semantic route for supported
  scalar and control-flow forms.
- Tests cover nearby same-feature cases, not only one known handoff fixture.
- x86 backend BIR/handoff tests are a usable acceptance surface: stale API or
  harness failures are repaired, supported forms run as supported, and any
  remaining unsupported forms are recorded as explicit boundaries.
- Any x86 handoff proof that later contributes to idea 120 consumes prepared
  label ids directly and rejects missing or drifted ids.

## Closure Notes

Closed after Step 5 completion review and backend regression guard.
Supported x86 prepared-module scalar and control-flow routes are recorded in
the active lifecycle notes and tests; unsupported forms remain explicit
contract-first boundaries rather than hidden skips. Prepared control-flow
handoff now consumes authoritative prepared label/branch/join/parallel-copy
identity and rejects missing or drifted metadata.

Closure evidence:
- `review/step5_completion_readiness_review.md` judged the plan on track, the
  source idea matched, and testcase-overfit risk low.
- `test_after.log` recorded the delegated Step 5 backend proof passing
  122/122 tests for `^backend_`.
- Close-time regression guard compared canonical `test_before.log` and
  `test_after.log`, improving from 110/122 to 122/122 with no new failures.
