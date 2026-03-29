# Parser Whitelist Boundary Audit Runbook

Status: Active
Source Idea: ideas/open/07_parser_whitelist_boundary_audit.md
Activated from: ideas/open/07_parser_whitelist_boundary_audit.md on 2026-03-29
Supersedes: ideas/open/04_std_vector_bringup_plan.md as the active debugging line

## Purpose

Shift the current parser debugging strategy from case-by-case libstdc++
reduction onto a narrower parser-hygiene audit of broad skip, recovery, and
start-probe rules that are making downstream failures expensive to localize.

## Goal

Identify the highest-risk parser boundaries that are not whitelist-led, tighten
the first batch with reduced repro coverage, and use that cleanup to make later
`std::vector` bring-up work more efficient.

## Core Rule

Do not treat "it eventually parses" as success if the path depends on broad
token swallowing or on recovery that can hide the real failure site.

## Read First

- [ideas/open/07_parser_whitelist_boundary_audit.md](/workspaces/c4c/ideas/open/07_parser_whitelist_boundary_audit.md)
- [ideas/open/04_std_vector_bringup_plan.md](/workspaces/c4c/ideas/open/04_std_vector_bringup_plan.md)
- [prompts/AGENT_PROMPT_EXECUTE_PLAN.md](/workspaces/c4c/prompts/AGENT_PROMPT_EXECUTE_PLAN.md)

## Current Target

- parser boundary helpers that decide "keep consuming" vs "stop now"
- parser recovery loops that can skip across declaration or scope boundaries
- `NK_EMPTY` discard paths that may be hiding missed parses

## Non-Goals

- do not try to rewrite all parser recovery in one slice
- do not silently continue broadening parser acceptance without reduced tests
- do not resume container-specific `<vector>` debugging until the current audit
  produces at least one tighter, reusable boundary fix

## Working Model

Current evidence says:

- broad parser rules are now a direct productivity bottleneck
- the latest reduced `<vector>` blocker was improved by tightening a
  `requires`-clause boundary, not by adding container-specific support
- the next useful work is to inventory and rank similar broad rules so future
  fixes are deliberate and repeatable

## Execution Steps

### Step 1: Inventory non-whitelist boundaries

Goal:
- enumerate the parser sites that still rely on broad token skipping, generic
  recovery, or permissive start probes

Actions:
- inspect constraint / `requires` skipping helpers
- inspect generic recovery helpers
- inspect start-condition probes such as `is_type_start()`
- inspect parse-and-discard branches returning `NK_EMPTY`

Completion Check:
- one curated inventory exists in the active todo state

### Step 2: Rank by risk and evidence

Goal:
- distinguish intentionally broad helpers from risky ones already implicated in
  real misparses

Actions:
- tag each inventoried site as acceptable breadth, suspicious breadth, or known
  bug
- attach concrete reduced repros or live failure paths where available
- prefer sites already linked to state drift or downstream error displacement

Completion Check:
- one ordered shortlist of the highest-risk tightening targets exists

### Step 3: Tighten the first batch

Goal:
- land the first reusable parser-hygiene fixes with narrow scope

Actions:
- convert the chosen rules to whitelist-led boundaries where practical
- avoid context-leaking reuse of broad probes when a local follow-set is safer
- preserve or improve diagnostics when tightening behavior

Completion Check:
- at least one risky boundary has been tightened without regressing targeted
  existing tests

### Step 4: Lock fixes into regression coverage

Goal:
- keep the audit results durable and reusable

Actions:
- add reduced regression tests for each tightened rule
- record remaining high-risk sites and next tightening candidates in
  `plan_todo.md`
- note when the `std::vector` bring-up should be revisited

Completion Check:
- reduced tests and a next-target shortlist exist in-tree / in todo state
