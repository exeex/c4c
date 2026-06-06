# Route Review: Idea 114 Prepared Outgoing Stack Argument Area

## Scope

- Active source idea: `ideas/open/114_prepared_outgoing_stack_argument_area_contract.md`
- Active plan: `plan.md`
- Review base: `3675f9914 [plan] Activate prepared outgoing stack area plan`
- Base rationale: this is the lifecycle activation commit for the active idea 114 runbook. The source idea was created one commit earlier, but the requested review scope is from activation through `HEAD`; later commits are execution/todo and implementation slices for the same active idea.
- Reviewed range: `3675f9914..HEAD`
- Commits since base: 5

## Findings

### Medium: closure validation evidence is incomplete

The diff itself is aligned, but the current root-level validation artifacts are not sufficient for closure. `todo.md` reports Step 5 proof using `test_after.log` and a broader `^backend_` subset, but `test_after.log` is absent from the root at review time. The only canonical root log present is `test_before.log`, which records a narrow `backend_aarch64_instruction_dispatch` pass, and there is also a non-canonical `test_baseline.log`.

This matters because the source idea's proof route asks for focused BIR/prealloc, prepared-printer, classification/lookup, AArch64 dispatch, and existing AArch64 c-testsuite stack-argument coverage without weakening expectations. The implementation touches shared prepared call-plan surfaces plus one target consumer, so closure should have fresh after-HEAD proof for the focused touched tests and either the documented broader backend subset in canonical form or an explicit AArch64 c-testsuite stack-argument pass.

References:
- `todo.md`: cites `test_after.log` for Step 5 and a broader backend subset.
- root artifacts: `test_before.log` present, `test_after.log` absent, `test_baseline.log` present.

## Alignment Review

The route matches the source idea. The diff adds `PreparedOutgoingStackArgumentArea` and stores it as an optional call-level fact on `PreparedCallPlan` in `src/backend/prealloc/calls.hpp:404` and `src/backend/prealloc/calls.hpp:417`. Shared prealloc computes the fact after all argument destination plans are populated, using the max extent across stack destinations rather than the first argument only, in `src/backend/prealloc/call_plans.cpp:228` and `src/backend/prealloc/call_plans.cpp:2756`.

Lookup support preserves the call-level fact by indexed call position without recomputing it at the lookup site, in `src/backend/prealloc/prepared_lookups.cpp:1225` and `src/backend/prealloc/prepared_lookups.cpp:3405`. Printer output exposes the call-level area independently from per-argument destination offsets in `src/backend/prealloc/prepared_printer/calls.cpp:433`.

AArch64 consumption is appropriately target-local. `src/backend/mir/aarch64/codegen/calls.cpp:573` now derives reservation bytes from `PreparedCallPlan::outgoing_stack_argument_area`, while `x16`, concrete `sub sp` / `add sp` records, and instruction ordering remain in AArch64 code at `src/backend/mir/aarch64/codegen/calls.cpp:5011` and `src/backend/mir/aarch64/codegen/calls.cpp:5166`. I did not find shared prealloc references to `x16` or target instruction sequencing.

I found no testcase-overfit pattern in the reviewed diff. There are no allowlist, unsupported, or expectation-downgrade rewrites in the range. The tests added/updated check a multi-stack-argument extent, lookup preservation distinct from a single lane, printer visibility, and AArch64 reservation driven by an area larger than the actual stack lane.

## Judgments

- Idea alignment: `matches source idea`
- Runbook transcription: `plan matches idea`
- Route alignment: `on track`
- Technical debt: `acceptable`
- Validation sufficiency: `needs broader proof`
- Reviewer recommendation: `continue current route`

## Closure Recommendation

Do not close solely on the current artifact state. Before closure, regenerate canonical after-HEAD validation evidence for the focused touched tests and the broader acceptance gate chosen by the supervisor. At minimum, that should include the BIR/prealloc call contract, prepared lookup helper, prepared printer, call-boundary classification, and AArch64 dispatch tests, plus explicit evidence that the relevant existing AArch64 stack-argument c-testsuite cases remain supported without expectation weakening.
