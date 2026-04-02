# Std Vector Success-Path And STL Throughput Follow-On

Status: Active
Source Idea: ideas/open/std_vector_success_path_and_stl_throughput_follow_on.md
Activated from: ideas/open/std_vector_success_path_and_stl_throughput_follow_on.md

## Purpose

Turn the remaining motivating STL validation pain into a measured, executable
 runbook without reopening the completed parser-debug failure-observability
 slice.

## Goal

Make the `std::vector` / `eastl::vector` motivating workflows usable for manual
 parser investigation by addressing the smallest validated gap among
 success-path visibility, throughput, and repeatable include-path setup.

## Core Rule

Keep this narrow and measurement-driven. Do not broaden into general parser
 optimization or another tracing redesign unless the data shows a direct need.

## Read First

- `ideas/open/std_vector_success_path_and_stl_throughput_follow_on.md`
- `ideas/closed/parser_debug_surface_for_vector_stl_work.md`
- `src/apps/c4cll.cpp`
- parser debug reporting and CLI entrypoints
- motivating files:
  - `tests/cpp/std/std_vector_simple.cpp`
  - `tests/cpp/eastl/eastl_vector_simple.cpp`
- existing EASTL workflow:
  - `tests/cpp/eastl/run_eastl_type_traits_simple_workflow.cmake`

## Current Targets

- establish a repeatable baseline for `std_vector_simple.cpp` timeout behavior
- establish a repeatable manual invocation for `eastl_vector_simple.cpp`
- decide whether the next shippable slice is:
  - a success-path parser summary surface
  - a targeted throughput fix
  - a workflow wrapper for motivating library cases
- validate the chosen slice with focused tests or workflow checks

## Non-Goals

- do not reopen failure-local token window work
- do not redesign parser debug infrastructure
- do not claim broad parser performance improvements without case-specific
  measurements
- do not silently absorb unrelated STL parse failures into this runbook

## Working Model

Implement this in narrow slices:

1. measure current behavior on the motivating files
2. choose the smallest fix that removes the most immediate validation friction
3. add focused coverage before implementation
4. re-run motivating workflows and nearby tests

## Execution Rules

- treat the motivating files as validation targets, not as a mandate to fix all
  library parsing gaps in one slice
- preserve the new parser-debug flags and current failure-output shape unless a
  step explicitly extends success-path reporting
- if `std_vector_simple.cpp` remains slow, record where time is spent before
  changing parser behavior
- if another independent parser feature gap appears, record it as a separate
  idea instead of silently widening this plan

## Ordered Steps

### Step 1: Baseline Motivating Workflows

Goal: produce a stable reproduction recipe and timing/error baseline for both
 motivating files.

Primary targets:

- `tests/cpp/std/std_vector_simple.cpp`
- `tests/cpp/eastl/eastl_vector_simple.cpp`
- `tests/cpp/eastl/run_eastl_type_traits_simple_workflow.cmake`

Actions:

- measure `std_vector_simple.cpp` with bounded `--parse-only` and, if useful,
  bounded parser-debug invocations
- confirm the required include-path recipe for `eastl_vector_simple.cpp`
- record whether the EASTL motivating case currently fails in preprocessing,
  parse-only, or later stages
- update `plan_todo.md` with the exact observed blockers and timings

Completion check:

- the repo contains a reproducible baseline for both motivating workflows

### Step 2: Choose The Smallest High-Value Slice

Goal: decide which single next change removes the most immediate STL
 investigation friction.

Primary targets:

- parser debug CLI/reporting path
- motivating workflow wrappers or tests
- measured hot path from Step 1

Actions:

- compare the value of a success-path summary mode against a targeted
  throughput fix or workflow wrapper
- prefer the option that improves manual investigation with the smallest code
  surface
- record the chosen slice and the rejected alternatives in `plan_todo.md`

Completion check:

- one bounded implementation slice is selected with a clear validation target

### Step 3: Add Focused Coverage For The Chosen Slice

Goal: lock the intended behavior before implementation.

Primary targets:

- focused internal parser-debug tests
- motivating workflow scripts or targeted CLI checks

Actions:

- add or update the narrowest test or workflow assertion that proves the chosen
  slice
- keep timeout expectations explicit when covering long-running motivating
  cases

Completion check:

- a focused failing or missing validation exists for the chosen slice

### Step 4: Implement The Chosen Slice

Goal: make the chosen success-path, throughput, or workflow improvement work
 without broad parser churn.

Primary targets:

- the smallest implementation surface justified by Steps 1-3

Actions:

- implement only the selected slice
- preserve existing failure-path parser-debug behavior
- avoid unrelated parser cleanup unless required by the slice

Completion check:

- the focused validation from Step 3 passes

### Step 5: Revalidate Motivating Cases

Goal: prove the chosen slice materially improves the motivating STL workflow.

Primary targets:

- `tests/cpp/std/std_vector_simple.cpp`
- `tests/cpp/eastl/eastl_vector_simple.cpp`
- nearby parser-debug or workflow tests

Actions:

- re-run the motivating commands with the chosen improvement in place
- run nearby targeted tests and the broader regression suite appropriate to the
  touched surface
- record any remaining separate initiatives back into `ideas/open/`

Completion check:

- the motivating STL workflow is measurably more usable without reopening the
  completed failure-observability effort

## Done Condition

This runbook is complete when:

- the repo has a repeatable motivating-case workflow for both STL examples
- the next highest-value friction point has been addressed in one narrow slice
- focused validation covers that slice
- any remaining separate work has been recorded as distinct open ideas
