# std::vector Bring-up Runbook

Status: Active
Source Idea: ideas/open/04_std_vector_bringup_plan.md
Activated from: ideas/open/04_std_vector_bringup_plan.md on 2026-03-29

## Purpose

Push `tests/cpp/std/std_vector_simple.cpp` forward by isolating the first real
libstdc++ header combination that causes parser state drift on the `<vector>`
include path.

## Goal

Turn the current "deep failure inside `<vector>`" frontier into one reduced,
actionable parser bug with a stable repro and regression test.

## Core Rule

Do not broaden this slice into general builtin-trait semantics work unless a
trait gap is proven to be the direct blocker for the reduced `<vector>` repro.

## Read First

- [ideas/open/04_std_vector_bringup_plan.md](/workspaces/c4c/ideas/open/04_std_vector_bringup_plan.md)
- [prompts/AGENT_PROMPT_EXECUTE_PLAN.md](/workspaces/c4c/prompts/AGENT_PROMPT_EXECUTE_PLAN.md)

## Current Target

- `tests/cpp/std/std_vector_simple.cpp`
- real system-header path rooted at `<vector>`
- parser-state drift that surfaces around `functional_hash.h` / `stl_bvector.h`

## Non-Goals

- do not implement broad libstdc++ semantic coverage
- do not treat the first visible header error as root cause without reduction
- do not fold the separate builtin-trait audit idea into this runbook

## Working Model

The current evidence says:

- `<type_traits>` alone parses
- `<bits/functional_hash.h>` alone parses
- a small combination including `type_traits`, `hash_bytes`, `functional_hash`,
  and `stl_bvector` still parses
- the failure likely depends on a larger include-chain interaction or parser
  state leak, not the first visible `_Arg` error site

## Execution Steps

### Step 1: Reconfirm the live frontier

Goal:
- verify the current failing `<vector>` repro and capture the first visible
  errors without assuming they are root cause

Actions:
- run the direct parse-only repro on `tests/cpp/std/std_vector_simple.cpp`
- capture the earliest failing headers and line numbers
- record any drift from the last known frontier in `plan_todo.md`

Completion Check:
- one current failure snapshot is recorded

### Step 2: Reduce the include chain

Goal:
- find the smallest header set on the `<vector>` path that still reproduces the
  failure

Actions:
- inspect the preprocessed / dependency include chain for `<vector>`
- construct reduced repros by adding headers in order and by binary search
- keep reductions rooted in the real include order when possible

Completion Check:
- one reduced header combination reliably reproduces the parser failure

### Step 3: Identify the first corruption point

Goal:
- distinguish the first parser-state corruption point from downstream symptom
  headers

Actions:
- compare failing and non-failing neighboring header combinations
- inspect the grammar construct introduced by the first bad combination
- map it to the parser subsystem most likely responsible

Completion Check:
- one concrete syntax shape is named as the next implementation target

### Step 4: Lock the reduction into tests

Goal:
- preserve the newly isolated frontier as a regression target before broader
  implementation

Actions:
- add the narrowest reduced parse test that captures the bug
- keep the test scoped to the isolated syntax / include interaction
- update `plan_todo.md` with the next parser fix slice

Completion Check:
- the reduced repro exists in-tree and the next code-change target is recorded
