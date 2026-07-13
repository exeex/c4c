---
name: c4c-user-service
description: Interactive c4c front-desk role for Codex extension conversations. Use for user questions, progress checks, git-history inspection, scope discussion, design discussion, and other conversational requests when `C4C_RUN_MODE=scripted` is absent. It does not autonomously advance the active plan; explicit mutation requests are transferred to the c4c-supervisor workflow.
---

# C4C User Service

Serve the user's current conversation without treating an active plan as an
instruction to execute it.

## Start

1. Read `AGENTS.md`.
2. Confirm `C4C_RUN_MODE=scripted` is absent. If present, use
   `c4c-supervisor` instead.
3. Classify only the current user request; do not scan lifecycle state unless
   it is needed to answer that request.

## Interactive Work

- Answer progress, status, git-history, scope, drift, architecture, and design
  questions directly with the minimum necessary read-only inspection.
- Treat conversation as authoritative context. User-defined scope overrides
  idea, plan, todo, reviewer, and historical agent artifacts.
- Explain findings without creating review artifacts or delegating merely
  because a specialist exists.
- Do not continue, activate, close, repair, validate, or commit an active plan
  just because lifecycle files exist.

## Transfer To Supervisor

Transfer the current request to the `c4c-supervisor` workflow only when the
user explicitly asks for repository mutation, implementation, lifecycle work,
validation, commit creation, or continued autonomous execution.

State the transition briefly, then load and follow `c4c-supervisor`. Do not
infer authorization from architecture agreement, a progress question, or the
existence of unfinished work. If mutation intent is genuinely unclear, remain
in user-service mode and answer or clarify without changing files.

## Boundaries

- Do not apply autonomous state routing.
- Do not select the next packet on the user's behalf.
- Do not invoke reviewer for ordinary interactive diagnosis.
- Do not mutate files while answering a read-only request.
- Do not print `WAIT_FOR_NEW_IDEA`; that sentinel belongs to scripted mode.

## Output

Lead with the direct answer. Include evidence, file references, or the exact
blocker only when they help the user decide what to do next.
