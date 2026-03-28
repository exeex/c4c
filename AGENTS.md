# AGENTS

This repo uses a small planning lifecycle for agent work.

## Layers

1. `ideas/open/*.md`: open idea inventory that agents should pay attention to
2. `ideas/closed/*.md`: archived completed ideas
3. [`plan.md`](/workspaces/c4c/plan.md): the single active runbook
4. [`plan_todo.md`](/workspaces/c4c/plan_todo.md): mutable execution state for the active runbook

## State Detection

Judge lifecycle state by file path, file existence, and todo completion:

1. If both [`plan.md`](/workspaces/c4c/plan.md) and [`plan_todo.md`](/workspaces/c4c/plan_todo.md) exist **and all items in `plan_todo.md` are marked complete**, the plan is finished. Follow `prompts/AGENT_PROMPT_CLOSE_PLAN.md` to close it.

2. If both [`plan.md`](/workspaces/c4c/plan.md) and [`plan_todo.md`](/workspaces/c4c/plan_todo.md) exist and there are incomplete items, there is one active plan. Follow `prompts/AGENT_PROMPT_EXECUTE_PLAN.md` to push the progress.

3. If only [`plan.md`](/workspaces/c4c/plan.md) exists (no `plan_todo.md`), there is a new active plan. Follow `prompts/AGENT_PROMPT_EXECUTE_PLAN.md` to push the progress.

4. If neither [`plan.md`](/workspaces/c4c/plan.md) nor [`plan_todo.md`](/workspaces/c4c/plan_todo.md) exists **and `ideas/open/` contains activatable ideas**, there is no active plan. Follow `prompts/AGENT_PROMPT_ACTIVATE_PLAN.md` to activate one.

5. If neither [`plan.md`](/workspaces/c4c/plan.md) nor [`plan_todo.md`](/workspaces/c4c/plan_todo.md) exists **and `ideas/open/` is empty**, there is nothing to do. Print the keyword `WAIT_FOR_NEW_IDEA` and end the conversation.

6. If only [`plan_todo.md`](/workspaces/c4c/plan_todo.md) exists without [`plan.md`](/workspaces/c4c/plan.md), the planning state is inconsistent and should be repaired before implementation work continues.

7. Agents should only scan `ideas/open/` for candidate work.
8. `ideas/closed/` is archive storage and should be ignored unless the task is historical review.

## Entry Points

Agent prompts live in [`prompts/`](/workspaces/c4c/prompts).

- [`prompts/AGENT_PROMPT_ACTIVATE_PLAN.md`](/workspaces/c4c/prompts/AGENT_PROMPT_ACTIVATE_PLAN.md): activate one idea into `plan.md`
- [`prompts/AGENT_PROMPT_EXECUTE_PLAN.md`](/workspaces/c4c/prompts/AGENT_PROMPT_EXECUTE_PLAN.md): execute the active `plan.md`
- [`prompts/AGENT_PROMPT_DEACTIVATE_PLAN.md`](/workspaces/c4c/prompts/AGENT_PROMPT_DEACTIVATE_PLAN.md): fold an active plan back into its source idea
- [`prompts/AGENT_PROMPT_SWITCH_PLAN.md`](/workspaces/c4c/prompts/AGENT_PROMPT_SWITCH_PLAN.md): deactivate one plan and activate another
- [`prompts/AGENT_PROMPT_CLOSE_PLAN.md`](/workspaces/c4c/prompts/AGENT_PROMPT_CLOSE_PLAN.md): close a completed active plan
- [`prompts/AGENT_PROMPT_PLAN_FROM_IDEA.md`](/workspaces/c4c/prompts/AGENT_PROMPT_PLAN_FROM_IDEA.md): regenerate `plan.md` from one `ideas/open/*.md`

## Rules

1. There must be at most one active plan, represented by the pair `plan.md` and `plan_todo.md`.
2. Every active [`plan.md`](/workspaces/c4c/plan.md) should identify its source file under `ideas/open/`.
3. [`plan_todo.md`](/workspaces/c4c/plan_todo.md) must match the same source idea as [`plan.md`](/workspaces/c4c/plan.md).
4. If execution discovers a separate initiative, write it into `ideas/open/` and switch lifecycle state instead of silently expanding the active plan.
5. Closing an idea means:
   - delete [`plan_todo.md`](/workspaces/c4c/plan_todo.md)
   - delete [`plan.md`](/workspaces/c4c/plan.md)
   - update the corresponding file in `ideas/open/` to mark it complete and optionally record leftover issues
   - move that idea file from `ideas/open/` to `ideas/closed/`

## Skills

- [`idea-to-runbook-plan`](/workspaces/c4c/.codex/skills/idea-to-runbook-plan/SKILL.md)
- [`plan-lifecycle`](/workspaces/c4c/.codex/skills/plan-lifecycle/SKILL.md)

---

## Git-Triggered Automation Framework

The repo includes a local, event-driven orchestration layer in `ops/`.

### Setup (run once after cloning)

```bash
bash ops/setup.sh
```

This configures `core.hooksPath = .githooks` so the `post-commit` hook fires automatically.

### How it works

```
commit
  -> .githooks/post-commit
  -> ops/orchestrator.py enqueue-event post-commit --head HEAD
  -> detects ideas/open/*.md added  ->  NewIdeaCreated event enqueued
  -> ops/orchestrator.py process-queue  (spawned async)
  -> planner handler creates plan/PLAN-NNN-<idea> branch
  -> initialises plan.md + plan_todo.md on that branch
```

### Architecture layers

| Layer        | File                       | Role                                     |
|--------------|----------------------------|------------------------------------------|
| Hook         | `.githooks/post-commit`    | Lightweight trigger, enqueues event      |
| Orchestrator | `ops/orchestrator.py`      | Maps Git diff → domain events, routes    |
| Rules        | `ops/rules.json`           | Event → agent routing table              |
| State        | `ops/state/`               | Event queue + log (gitignored at runtime)|
| Execution    | `ops/execution/PLAN-NNN/`  | Per-plan notes (lives on plan branch)    |

### Domain events (Phase 1)

| Event             | Trigger                              | Handler         |
|-------------------|--------------------------------------|-----------------|
| `NewIdeaCreated`  | `A ideas/open/*.md` in commit        | planner         |
| `PlanCompleted`   | `plan_todo.md` all items checked     | merge_validator |

### Branching convention

- `main` — canonical shared truth only (ideas, AGENTS.md, src, tests)
- `plan/PLAN-NNN-<slug>` — one branch per idea execution attempt
- `task/TASK-NNN-<slug>` — optional sub-branches for parallel work

### CLI reference

```bash
# Inspect what would fire on the current HEAD
python3 ops/orchestrator.py enqueue-event post-commit --head HEAD

# Drain the queue manually
python3 ops/orchestrator.py process-queue

# Inspect queued events
python3 ops/orchestrator.py show-queue

# Inspect event history
python3 ops/orchestrator.py show-log
```
