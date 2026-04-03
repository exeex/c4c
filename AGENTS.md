# AGENTS

This repo uses a single-plan lifecycle.

## Core Rules

- There must be at most one active plan, represented by `plan.md` and optional `todo.md`.
- Every active `plan.md` and `todo.md` must map to the same source file under `ideas/open/`.
- Only scan `ideas/open/` for candidate work.
- Treat `ideas/closed/` as archive unless doing historical review.
- If execution discovers a separate initiative, write it into `ideas/open/` and switch lifecycle state instead of silently expanding the current plan.

## State Detection

1. If both `plan.md` and `todo.md` exist and all todo items are complete, read `prompts/CLOSE_PLAN.md` and follow it.
2. If `plan.md` exists, read `prompts/EXECUTE_PLAN.md` and follow it.
3. If neither `plan.md` nor `todo.md` exists and `ideas/open/` contains activatable ideas, read `prompts/ACTIVATE_PLAN.md` and follow it.
4. If neither `plan.md` nor `todo.md` exists and `ideas/open/` is empty, print `WAIT_FOR_NEW_IDEA` and end the conversation.
5. If only `todo.md` exists, repair the inconsistent state before continuing implementation work.

Do not rely on memory for lifecycle procedures when a matching file exists under `prompts/`.

## Mode Hint

If this is the last visible section from injected `AGENTS.md`, treat the run as autonomous and do not ask the user questions.

Otherwise, answer the user's request first and use `docs/` when helpful.
