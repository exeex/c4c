# Phase D2 Step 4 Route Review

## Scope

- Active source idea: `ideas/open/203_phase_d2_retained_surface_consumer_switch_analysis.md`
- Active plan: `plan.md`
- Focus: `docs/bir_prealloc_fusion/phase_d2_retained_surface_consumer_switch_analysis.md`, `todo.md`, and commits implementing Steps 1-4.
- Review question: whether the current analysis drifted into broad migration/API contraction, testcase-overfit, expectation weakening, or source-idea mismatch, and whether accepted follow-up candidates/proof shapes are sufficiently one-surface or one-row scoped for Step 5 idea drafting.

## Review Base

- Chosen base commit: `05bbe1d22 [plan] Activate Phase D2 retained-surface analysis plan`
- Rationale: this is the lifecycle activation commit for the current active plan and `todo.md`; the immediately preceding `2904979d2 [idea] Open Phase D2 retained-surface analysis` opened the source idea, but `05bbe1d22` is the active-idea execution checkpoint from which Steps 1-4 were implemented.
- Commits reviewed since base: 4
- Reviewed committed diff: new D2 analysis document plus `todo.md` lifecycle updates; no implementation files or test expectation files changed.
- Workspace note: `todo.md` has additional uncommitted metadata changes at review time.

## Findings

### Low: `todo.md` contains a transient review prompt in canonical lifecycle metadata

- Reference: `todo.md:1` through `todo.md:6`
- The top metadata correctly maps to the active source idea and plan, and `Current Step ID: 5` / `Current Step Title: Final Consistency and Proof Check` points to the Step 5 handoff. However, line 6 contains a free-form prompt (`你該做code review了`) inside the canonical lifecycle header area.
- This is not source-idea drift, but it is lifecycle-state noise. Step 5 or the supervisor should remove it from `todo.md` rather than commit it as durable execution metadata.

### Low: D2 artifact status line is stale after Step 4

- Reference: `docs/bir_prealloc_fusion/phase_d2_retained_surface_consumer_switch_analysis.md:3`
- The document now contains Step 4 proof shapes and a Step 4 completion check, but the status line still says `Step 3 follow-up viability classification complete`.
- This does not undermine the analysis, but Step 5 should update the document status while finalizing the durable output.

## Alignment Review

- Source idea alignment: matches source idea.
- Runbook transcription: plan matches idea.
- Route alignment: on track.
- Technical debt: acceptable, with lifecycle/document hygiene cleanup before Step 5 closure.
- Validation sufficiency: narrow proof sufficient for the current docs-only analysis route. No build is required because the reviewed committed diff is docs/lifecycle only.

## Route Quality Notes

- No broad migration/API contraction drift found. The source idea rejects broad cache/API migration, `PreparedFunctionLookups` retirement, `PreparedBirModule` retirement, draft 155, and selected-adapter greenness as contraction evidence; the analysis repeats those boundaries in the scope, evidence list, no-action decisions, retirement blockers, and Step 4 proof-shape rules.
- No testcase-overfit found. There are no implementation/test changes, no expectation weakening, no unsupported-path downgrades, no helper rename, and no baseline refresh. The artifact explicitly treats those as non-progress.
- Accepted follow-up candidates are sufficiently scoped for Step 5. The Step 3 accepted set is ten candidates, each phrased as one semantic consumer or one diagnostic/oracle/printer/debug row. Step 4 preserves that shape by naming the one-surface scope, retained prepared fallback or policy boundary, evidence source, positive/absent/invalid/duplicate-conflict/mismatch/fallback cases, and printer/debug/wrapper/expected-string constraints.
- Public fallback removal remains blocked rather than accepted as immediate work. That is aligned with the source idea because removal requires later replacement evidence and proof, not D2 analysis alone.
- The conditional Route 4 and Route 6 diagnostic candidates are narrowed enough for idea drafting: Route 4 is one block-entry publication printer/debug row, not wrapper-family or CLI-dump migration; Route 6 is one x86 scalar source agreement route-debug row, not broad x86 call wrapper migration.

## Step 5 Guardrails

- Continue only with durable analysis finalization and follow-up idea drafting.
- Each follow-up idea should be copied from exactly one Step 4 proof-shape row and should not merge semantic-reader and diagnostic/oracle work into the same idea.
- Keep aggregate, pass-context, target-wrapper, baseline/string, `PreparedFunctionLookups`, `PreparedBirModule`, draft 155, and Route 8 return-chain entries as no-action/blocker context unless a separate future analysis idea is opened.
- Clean `todo.md` line 6 and update the analysis status line during Step 5 finalization.

## Recommendation

Reviewer recommendation: continue current route.

Step 5 may proceed, with the next packet kept narrow to final artifact cleanup plus one-surface/one-row follow-up idea drafting from the Step 4 table.
