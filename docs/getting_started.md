# Getting Started with c4c

This document is the first-stop onboarding guide for people who are new to this repository.

## What This Project Is

`c4c` is a compiler-oriented repository with an agent workflow layered on top of normal development work. In practice, there are two things to understand:

1. The codebase itself: compiler/frontend/backend work, debugging, tests, and integration.
2. The repo workflow: ideas live under `ideas/open/`, active work is represented by `plan.md` and `todo.md`, and completed ideas move to `ideas/closed/`.

## How To Work Here

Most people should start in an interactive agent session instead of the autonomous loop.

Interactive mode is best when you want to:

- understand the codebase
- explore how a feature should be implemented
- debug a failure together
- ask architecture or integration questions

Autonomous loop mode is best when you already know the initiative and want the agent to keep pushing the active plan with minimal supervision.

## Good First Questions

If you are new, these prompts work well:

- `I just started looking at this project. What can I do here?`
- `What is the current active plan and what should I read first?`
- `I want to implement <feature>. Where in the codebase should we start?`
- `I want to connect the c4c compiler to my custom hardware. What parts of the system do I need to understand first?`

## Common Task Types

### 1. General exploration

Use this when you are still learning the repository.

Expected outcome:

- a short description of the project areas
- the current plan state
- suggested files to read next

### 2. Implementing a feature

Use this when you have a concrete goal such as a new frontend feature, compiler behavior change, or test improvement.

Expected outcome:

- the likely implementation area
- an initial design sketch
- a proposed change/test sequence

### 3. Integrating custom hardware

Use this when you want to target a custom ISA, accelerator, or board-specific environment.

Expected outcome:

- which compiler stages are affected
- what target description / lowering / codegen pieces matter
- what bring-up plan to follow

## Planning Lifecycle

This repo uses a lightweight planning lifecycle:

- `ideas/open/`: candidate initiatives
- `plan.md`: the one active runbook
- `todo.md`: mutable checklist for the active runbook
- `ideas/closed/`: archived completed initiatives

If you are just asking a question, the agent should answer the question first.
If you are asking it to execute work autonomously, the agent should follow the lifecycle rules in `AGENTS.md`.

## Recommended Next Docs

- [use_cases.md](/workspaces/c4c/docs/use_cases.md)
- [AGENTS.md](/workspaces/c4c/AGENTS.md)

## Draft Scope

This is an onboarding draft. Expand it over time with:

- architecture maps
- file-by-file reading guides
- backend bring-up notes
- examples of successful feature work
