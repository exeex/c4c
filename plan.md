# Backend Port Roadmap Activation Runbook

Status: Active
Source Idea: ideas/open/__backend_port_plan.md
Activated from: ideas/open/__backend_port_plan.md

## Purpose

Turn the remaining backend umbrella roadmap back into one executable child idea so implementation work can continue under the repo's one-plan lifecycle.

## Goal

Extract the next highest-priority unfinished backend child slice from the umbrella roadmap and convert it into a narrow active runbook.

## Core Rule

Do not implement backend code from this umbrella runbook. This slice is only for restoring a narrow executable child plan and switching execution back onto that child.

## Read First

- `ideas/open/__backend_port_plan.md`
- `prompts/AGENT_PROMPT_PLAN_FROM_IDEA.md`
- `prompts/AGENT_PROMPT_SWITCH_PLAN.md`

## Current Targets

- identify which roadmap child should be re-opened next
- write one durable child idea under `ideas/open/`
- regenerate `plan.md` from that child idea and reset `plan_todo.md`

## Non-Goals

- editing implementation sources under `src/`
- bundling multiple backend initiatives into one plan
- reopening completed narrow ideas without a clear missing follow-on

## Working Model

- treat `ideas/open/__backend_port_plan.md` as durable roadmap context, not as the long-term execution target
- prefer the highest-priority missing child from the roadmap ordering when multiple candidates seem plausible
- preserve the umbrella file as the roadmap and move execution back onto a child plan as quickly as possible

## Execution Rules

- keep the new child idea narrower than the umbrella roadmap
- record the chosen child idea under `ideas/open/` before replacing this umbrella runbook
- if the next child is unclear from repo state, record the ambiguity in the new idea instead of inventing implementation scope

## Step 1: Choose The Next Child Slice

Goal: determine the next executable backend child idea to restore from the roadmap.

Actions:

- inspect the roadmap priority order and any currently open backend idea files
- identify the highest-priority child idea that is not already represented under `ideas/open/`
- keep the choice to one mechanism family

Completion check:

- one child slice is chosen and named explicitly
- the choice is justified from the roadmap ordering and current open-idea inventory

## Step 2: Write The Child Idea

Goal: restore one narrow backend child idea under `ideas/open/`.

Actions:

- create one `ideas/open/*.md` child idea for the chosen slice
- capture scope, validation intent, non-goals, and relationship to the umbrella roadmap
- keep the child idea implementation-oriented enough to become the next active runbook

Completion check:

- the new child idea exists under `ideas/open/`
- it is narrower than the umbrella and points back to the roadmap

## Step 3: Switch Activation To The Child Runbook

Goal: stop using the umbrella as the active runbook and activate the new child idea.

Actions:

- regenerate `plan.md` from the new child idea
- reset `plan_todo.md` to the child plan
- keep `Status: Active` and `Source Idea:` aligned across both files

Completion check:

- `plan.md` and `plan_todo.md` both point at the new child idea
- the umbrella roadmap remains only durable context, not the active execution target
