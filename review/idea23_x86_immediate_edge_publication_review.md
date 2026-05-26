# Idea 23 x86 Immediate Edge Publication Review

Active source idea: `ideas/open/23_x86_prepared_edge_publication_remaining_home_coverage.md`

Review question: Does the current implementation path still match idea 23, avoid testcase overfit, and have sufficient validation to proceed to the Step 5 RISC-V readiness handoff?

## Base Selection

Chosen base commit: `f23fff39a [plan] Activate remaining x86 edge home coverage plan`

Why this base: this is the lifecycle activation checkpoint for the currently active source idea 23 and created the active `plan.md`/`todo.md` state. The later lifecycle commit `15220e143` only recorded Step 1 inventory, and `e775c5f7a` is the implementation slice under review.

Commits since base: 2

Reviewed scope:

- `git diff f23fff39a..HEAD`
- current uncommitted `todo.md` Step 4 validation note
- current `test_after.log` backend-bucket evidence

## Findings

No blocking findings.

Watch, Step 5 must not turn the validation result into an unconditional RISC-V readiness claim. The current slice intentionally leaves `PointerBasePlusOffset -> Register` and all `StackSlot` destinations fail-closed, which is consistent with idea 23 only if the final handoff documents that rationale and makes a concrete readiness decision rather than implying the x86 surface is complete. Context: `ideas/open/23_x86_prepared_edge_publication_remaining_home_coverage.md` rejects readiness claims while selected remaining homes retain unsupported behavior without documented rationale; current `todo.md` lines 33-45 already carry the right warning.

## Alignment Review

The implementation matches the source idea. The code extends a semantic x86 source-home family by accepting `PreparedValueHomeKind::RematerializableImmediate` only when `immediate_i32` is present, and renders the x86 operand in the existing edge-publication source operand helper. It does not add edge-name, label-name, or testcase-name matching. See `src/backend/mir/x86/prepared/dispatch.cpp` lines 22-24.

The tests are not expectation-only progress. The new positive test mutates the fixture into a rematerializable-immediate source home, then checks that the helper returns the shared publication pointer, preserves the prepared value ids and source home kind, and emits `mov ebx, 42`. See `tests/backend/bir/backend_x86_prepared_decoded_home_storage_test.cpp` lines 702-749.

The negative coverage directly addresses idea 23's authority and fail-closed requirements. The tests verify no emission without shared lookups, preserve fail-closed behavior for pointer-base source homes, reject malformed immediate homes without `immediate_i32`, and keep stack-slot destinations unsupported. See `tests/backend/bir/backend_x86_prepared_decoded_home_storage_test.cpp` lines 753-763 and 797-849.

The current uncommitted `todo.md` validation note is accurately scoped. It records the focused 79-test proof plus matching regression guard before commit, then records the later 162-test backend bucket as broader validation only, not as a mismatched before/after regression comparison. See `todo.md` lines 17-29.

## Judgments

Idea-alignment judgment: matches source idea

Runbook-transcription judgment: plan matches idea

Route-alignment judgment: on track

Technical-debt judgment: acceptable

Validation sufficiency: narrow proof sufficient for Step 5 handoff; broader backend proof is also green

Reviewer recommendation: continue current route

## Step 5 Guidance

Proceed to Step 5. The handoff should say that x86 now covers stack-source, register-source, and rematerializable-immediate source homes to register destinations through shared `edge_publications`, while pointer-base source homes and stack-slot destinations remain explicitly fail-closed. The readiness decision can be affirmative only if those remaining fail-closed homes are judged non-blocking for opening a separate RISC-V consumer idea; otherwise Step 5 should recommend another x86 coverage slice.
