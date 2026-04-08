# Agent Prompt: Delegated Subagent Work

Last updated: 2026-04-08

## Mission

Execute a narrow delegated slice on behalf of a parent agent without taking
ownership of the repo-wide plan lifecycle.

This prompt is for subagents and worker lanes, not for the human-facing
mainline agent.

## Required Inputs

Before editing:

1. Read the parent task packet carefully.
2. Read [`plan.md`](../plan.md) when the delegated task depends on the active
   plan context.
3. Read the worker-specific file the parent points you at, if any:
   - `todoA.md`
   - `todoB.md`
   - `todoC.md`
   - `todoD.md`
   - or another explicitly named worker packet

## Ownership Model

1. The parent agent owns repo-wide coordination, broad validation, planning
   files, and final commit decisions unless it explicitly delegates one of those
   responsibilities.
2. Treat worker packet files such as `todoA.md` or `todoB.md` as delegated task
   descriptions, not as the authoritative plan lifecycle state.
3. Do not rewrite [`plan.md`](../plan.md) or [`todo.md`](../todo.md) unless the
   parent task explicitly asks for planning work.
4. Stay inside the file ownership named by the parent task whenever possible.
5. If the delegated slice turns out to require a cross-ownership refactor, stop
   expanding scope and hand the blocker back to the parent agent.

## Required Flow

1. Restate the owned slice to yourself from the parent task packet.
2. Inspect only the code and tests needed for that slice.
3. Make the smallest coherent change that advances the delegated goal.
4. Run only the narrowest validation needed for the files you changed:
   - prefer `.o` target builds for production sources
   - prefer a single owned test object or test binary when working in tests
5. Do not run the full repo build or broad `ctest` sweep unless the parent task
   explicitly requires it.
6. Do not create the final integration commit unless the parent task explicitly
   asks for it.
7. Return:
   - files changed
   - local validation run
   - important assumptions
   - blockers or follow-up notes for the parent agent

## Guardrails

1. Do not silently widen scope into another worker lane.
2. Do not edit another worker's packet file.
3. Do not take over final integration, final regression, or lifecycle closure
   unless explicitly delegated.
4. If local validation fails and the fix belongs outside your ownership, stop
   and report the blocker instead of thrashing across unrelated files.
5. If the parent task conflicts with repo-wide lifecycle files, the parent task
   controls your slice and the parent agent is responsible for reconciling the
   lifecycle state later.
