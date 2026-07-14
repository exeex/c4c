# Supervisor Lifecycle Operations

Use this workflow for lifecycle state, source-scope decisions, blockers,
switches, resumption, closure, and terminal routing. `c4c-plan-owner` performs
the lifecycle mutations; the supervisor supplies evidence, reviews the diff,
and creates the final commit.

## State Detection

1. Read `plan.md` and `todo.md` when present and confirm both name the same
   source under `ideas/open/`.
2. If only one exists, delegate plan-owner to repair the inconsistent state.
3. If both exist with incomplete work, remain in execution mode.
4. If both exist and the runbook is exhausted, ask plan-owner to decide close,
   deactivate, replace, or repair. Exhaustion alone is not source completion.
5. If neither exists, enumerate every file under `ideas/open/` before choosing
   activation or a terminal response.

File state selects the lifecycle operation. If an active pair already names
idea A, a request concerning idea B is a switch, never a second activation,
regardless of the caller's wording.

## Activate Or Resume

Use plain activation only when no active `plan.md` / `todo.md` pair exists and
the target has no prior execution state to recover.

Delegate plan-owner to:

1. read the target source idea;
2. derive `plan.md` with `idea-to-runbook-plan`;
3. create canonical `todo.md` metadata and executor-compatible skeleton;
4. keep implementation work out of the lifecycle slice.

If the target was active before, use resume rather than fresh activation.
Require plan-owner to read the source's resumption record and recover the last
historical `plan.md` / `todo.md` pair linked to that source. Preserve completed
work and point `todo.md` at the exact recorded return step; never reset a
previously active idea to Step 1 merely because another idea replaced its
runbook files.

## Out-Of-Scope Work And Blockers

When execution discovers required work outside the active source idea:

1. Do not absorb it into the active idea or dispatch it to an executor under
   the current runbook.
2. Delegate plan-owner to create a distinct durable idea under `ideas/open/`.
3. If it does not block the active step, record the separate idea and continue
   the current route.
4. If it blocks the active step, require one atomic lifecycle operation:
   create the blocker idea, preserve the parent's resumable execution state,
   and switch to the blocker.

The plan-owner packet must include the outgoing source, current step ID/title,
last accepted progress, completed steps, blocker and scope boundary, exact
return point, remaining next action, and accepted proof/commit references.

## Deactivate

Before removing or replacing either active runbook file, require plan-owner to
read `plan.md`, `todo.md`, and the linked source and write a compact resumption
record into the source idea containing:

- last accepted progress and completed runbook steps
- interrupted `Current Step ID` and `Current Step Title`
- blocker and why it is outside the source scope, when applicable
- exact return point and remaining next action
- accepted proof and implementation commit references

The record must be sufficient to reconstruct execution without the outgoing
`plan.md` or `todo.md`. A note that only says "blocked by idea N" is invalid.

## Switch

Switch is deactivate-then-activate as one lifecycle operation:

1. Snapshot the outgoing `plan.md`, `todo.md`, and source idea.
2. Write and verify the outgoing source's resumption record.
3. Create the new blocker source first when the target was newly discovered.
4. Deactivate the outgoing plan only after its execution state is durable.
5. Generate target `plan.md` from the target source.
6. Reset `todo.md` to the target source and current target step.
7. Verify the active pair names exactly one open source.

The supervisor must compare the outgoing runbook pair with the source
resumption record. Reject a switch that loses completed work, interrupted step,
blocker, exact return point, remaining action, or accepted proof.

After the blocker closes, resume the parent from its recorded return point and
retry the interrupted step or close decision.

## Close And Blocker Loop

When a runbook is exhausted, delegate the semantic close decision to
plan-owner and supply accepted supervisor proof.

- `close accepted`: inspect the lifecycle diff, run required final validation,
  and commit the closure.
- `close rejected / repair-current-route`: have plan-owner repair or replace
  the runbook, then return to bounded execution.
- `close rejected / separate-blocker`: create the separate open idea and switch
  through the operation above, then resume the parent afterward.
- evidence-backed negative/no-change/superseded route: archive only with an
  explicit disposition and a named open successor for remaining durable
  intent.

Never retire a runbook and leave its open idea without an executable repair
route or named successor.

## Terminal Routing

When neither active file exists:

- if `ideas/open/` is nonempty, delegate plan-owner to activate/resume an
  executable idea or resolve the earliest blocked dependency;
- emit exact `WAIT_FOR_NEW_IDEA` only when `ideas/open/` is empty and no
  unresolved durable intent requires a successor.

An open-but-blocked inventory is supervisor work, not a wait condition.

## Lifecycle Packet And Review

Every plan-owner packet names lifecycle trigger/objective, owned files,
do-not-touch files, accepted proof, done condition, and exact ambiguity to
report if blocked.

Before committing, inspect the handoff, `git diff`, source/runbook linkage, and
preserved resumption state. Stage only the coherent lifecycle slice and
preserve unrelated user files.
