# Review A: Post-337 Backend Regex Inventory Route

## Scope

Requested review of whether the current active idea 295 route should continue
with Step 2 classification from `test_after.log`, or whether lifecycle state
should be rewritten before further execution.

Active source idea path:
`ideas/open/295_backend_regex_failure_family_inventory.md`

Chosen base commit: `033d19aa0 [plan] activate backend regex inventory`

Why this base is correct: it is the latest activation commit for idea 295 after
closed idea 337, and it created the current active umbrella `plan.md` and
`todo.md`. The packet explicitly allowed this checkpoint if multiple idea 295
activations made base selection ambiguous.

Commit count since base: 1 committed change (`ecf313ab9 [todo_only] capture
post-337 backend regex inventory`), plus the current uncommitted `todo.md`
metadata edit.

Reviewed diff: `git diff 033d19aa0..HEAD`, current dirty `todo.md`, active
plan/todo, source idea 295, closed ideas 334-337, and `test_after.log`.

## Findings

### Medium: `todo.md` Step Metadata Now Points At Repair, Not Classification

`todo.md:4-5` says the current step is Step 2,
`Repair The Classified Operand-Fact Owner`. That conflicts with the active
runbook's Step 2 title and goal, `Classify Residual Failures`, at
`plan.md:95-120`, and with the source idea/runbook core rule that this umbrella
must not implement repairs before a focused lifecycle switch (`plan.md:18-22`,
`plan.md:144-160`).

The body of `todo.md` is otherwise aligned with classification:
`todo.md:59-63` says to classify the residual failures from `test_after.log`
and not split an owner until classification evidence is recorded.

This is a lifecycle metadata defect, not an implementation route failure. It
should be repaired before delegating the next executor packet because executor
routing depends on the regex-friendly `Current Step ID` and
`Current Step Title` fields.

### Medium: `test_after.log` Is Good Classification Input, But Not Enough To Reopen Closed Owners

The current log supports continuing Step 2 classification:

- `todo.md:17-25` records 354 selected, 325 passed, 26 failed, 3 timed out, with
  local/internal backend tests passing and all failures in external
  `c_testsuite_aarch64_backend_*`.
- `test_after.log:1213-1268` confirms 29 failed/timed-out tests out of 354 and
  lists the external AArch64 failures.
- The log includes first-stage tags needed for an initial bucket pass, for
  example scalar-cast machine-printer diagnostics at `test_after.log:786-808`,
  `test_after.log:922-930`, `test_after.log:947-969`, runtime nonzero at
  `test_after.log:304-307` and `test_after.log:477-491`, runtime mismatch at
  `test_after.log:892-918` and `test_after.log:1081-1133`, and timeouts at
  `test_after.log:1206-1210`.

However, the log alone should not reopen closed ideas 334-337 or select a
focused owner. It does not include generated assembly or first-bad generated-code
evidence for the runtime failures. That matters because some current failures
overlap prior idea 336 membership, such as `00112`, `00159`, and `00170`
(`test_after.log:477-491`, `test_after.log:892-918`,
`test_after.log:975-1003`). Those failures are not enough to reopen return
publication from counts or names alone.

### Low: Closed-Owner Boundaries Are Not Currently Contradicted

The current route does not show testcase-overfit in the post-337 diff: since
`033d19aa0`, only `todo.md` inventory/progress changed in committed history,
and the current dirty state is another `todo.md` metadata edit. There are no
post-337 implementation or expectation changes to review as overfit.

The current log also gives useful boundary checks:

- Closed idea 334's representatives `00164` and `00214` pass in the current
  backend-regex run (`test_after.log:325`, `test_after.log:499`), matching the
  closure note that the old scalar `mul`/`add` printer diagnostics were gone
  (`ideas/closed/334_aarch64_scalar_machine_node_operand_fact_printing.md:94-112`).
- Closed idea 335's representative `00164` passes (`test_after.log:499`), which
  does not contradict its local-slot publication closure
  (`ideas/closed/335_aarch64_uninitialized_local_slot_runtime_residual.md:77-94`).
- Closed idea 337's representative `00168` passes (`test_after.log:518`), which
  does not contradict its callee-saved scalar-home closure
  (`ideas/closed/337_aarch64_callee_saved_scalar_home_preservation.md:68-89`).

Idea 336 is more nuanced: some focused representatives pass after the closure,
but residual names from the original 22-case bucket still appear in the current
failure list. That is a proof-sufficiency watch item for Step 2 classification,
not a reason to reopen 336 without generated-code evidence. The active plan
already says closed owners through 337 stay closed unless fresh generated-code
or proof evidence contradicts a boundary (`plan.md:40-41`, `plan.md:112-115`).

### Low: The Hook Reminder Reveals Cadence Pressure, Not A Route Reset

`scripts/plan_review_state.py show` reports `code_review_pending: true`,
`counter: 6`, and `review_limit: 6` for current Step 2. The reminder itself
does not prove route drift. It usefully surfaced the stale Step 2 title and the
need to keep classification separate from repair. After the todo metadata is
repaired, this does not require a plan rewrite.

## Judgments

Idea-alignment judgment: `matches source idea`

The intended route remains idea 295's umbrella inventory/classification route:
capture the post-337 backend regex surface, classify residual failures, compare
against closed owners using evidence, and split before implementation.

Runbook-transcription judgment: `plan matches idea`

`plan.md` accurately transcribes the source idea. The problem is in `todo.md`
execution metadata, not in the plan.

Route-alignment judgment: `drifting`

The actual next action should still be Step 2 classification, but the current
step title is implementation-flavored and can misroute the next executor if
left as-is.

Technical-debt judgment: `action needed`

Action is limited to lifecycle scratchpad repair: update `todo.md` Step 2 title
and packet wording to classification, not repair. No source idea rewrite is
needed.

Validation sufficiency: `needs broader proof`

`test_after.log` is sufficient input for classification, but not sufficient to
select or reopen a focused owner. Any candidate bucket that overlaps closed
ideas 334-337 needs generated-code, diagnostic, ABI, or lowering evidence before
Step 3 owner selection.

Reviewer recommendation: `rewrite plan/todo before more execution`

Interpret this as a todo-only rewrite unless the plan owner sees additional
state damage: keep active idea 295 and `plan.md`, but repair `todo.md` so
`Current Step Title` is `Classify Residual Failures` and the next packet is
explicitly classification-only from `test_after.log`. After that, continue the
current route with a narrow Step 2 classification packet. Do not implement under
the umbrella, do not reopen closed owners from counts, and do not split a new
owner until classification evidence is recorded.
