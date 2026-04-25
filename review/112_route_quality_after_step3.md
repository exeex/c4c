# Route Quality Review: Idea 112 After Step 3

Review base: `95170812` (`[plan] Activate LIR backend legacy type surface audit`)

Base selection rationale: this commit adds the current `plan.md` and `todo.md`
for `ideas/open/112_lir_backend_legacy_type_surface_readiness_audit.md`; the
historical plan content matches the active plan title and source idea. Later
commits are report-only audit packets, not lifecycle resets or reviewer
checkpoints.

Commits since base: 3

Reviewed range: `95170812..HEAD`, plus current working-tree `todo.md` change.

Diff scope observed:
- `review/112_lir_backend_legacy_type_surface_readiness_audit.md`
- `todo.md`

## Findings

### High: `todo.md` prematurely marks the runbook as exhausted while Steps 4 and 5 remain

`todo.md:4` and `todo.md:5` set `Current Step ID: none` and
`Current Step Title: none`, and `todo.md:20` through `todo.md:23` tell the
supervisor to decide whether to close, retire, or expand the active runbook.
That conflicts with the active plan, which still has Step 4 at `plan.md:149`
through `plan.md:167` for proof gaps and validation recommendations, and Step 5
at `plan.md:169` through `plan.md:188` for final artifact completion and sanity
check.

The report artifact itself also stops at Step 3 scope mapping. It records some
proof gaps at `review/112_lir_backend_legacy_type_surface_readiness_audit.md:115`
through `review/112_lir_backend_legacy_type_surface_readiness_audit.md:125`,
but it does not yet provide the Step 4 focused validation buckets or broader
validation checkpoints required by `plan.md:157` through `plan.md:163`.

Impact: continuing from the current `todo.md` state risks treating a partial
audit as acceptance-ready. This is route drift in lifecycle state, not evidence
that the source idea or plan is wrong.

Recommendation: rewrite or repair `todo.md` before more execution so the
current packet advances to Step 4, then Step 5. No source idea rewrite is
needed, and `plan.md` only needs changes if the plan owner intentionally
collapses Steps 4 and 5 into a revised runbook.

### Medium: The Step 3 report is directionally aligned but not yet acceptance-complete

The audit report covers the requested Step 1 inventory, Step 2 classifications,
and Step 3 ownership split for ideas 113, 114, and 115. It also preserves the
MIR boundary and avoids implementation cleanup. However, the source idea's
required output includes proof gaps and recommended validation subsets, and the
active plan makes those a separate Step 4. The artifact should not be treated as
the final idea-112 deliverable until that section is added.

Impact: not technically misleading if read as "Step 3 follow-up scope mapped";
misleading only if promoted to final acceptance without Step 4/5.

Recommendation: continue the report-only route after `todo.md` repair. The next
executor packet should add actionable validation recommendations for aggregate,
HFA, sret, variadic, global-init, memory-addressing, GEP, initializer,
`va_arg`, byval/byref, verifier, and printer proof areas.

## Non-Findings

- No testcase-overfit evidence was found. The diff is report-only and does not
  change implementation code, tests, expectations, compile targets, or backend
  behavior.
- No source-idea drift was found. The report's classifications use the idea's
  category names and keep MIR/aarch64 as `planned-rebuild` residue.
- No excessive plan churn was found. There are three report-only commits since
  activation and no later `plan.md` rewrite.

## Judgments

Plan-alignment judgment: `drifting`

Idea-alignment judgment: `matches source idea`

Technical-debt judgment: `watch`

Validation sufficiency: `needs broader proof`

Reviewer recommendation: `rewrite plan/todo before more execution`

Required rewrite scope: `todo.md` repair is sufficient unless the plan owner
chooses to revise the runbook. The active plan and source idea remain suitable
for the report-only audit route.
