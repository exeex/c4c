# EASTL Container Bring-Up Runbook

Status: Active
Source Idea: ideas/open/47_eastl_container_bringup_plan.md
Activated from: ideas/open/47_eastl_container_bringup_plan.md

## Purpose

Turn the reopened EASTL bring-up idea into an execution runbook for small,
recipe-driven frontend slices that keep EASTL as the primary library pressure
source without expanding into broad STL work.

## Goal

Make the EASTL test ladder under `tests/cpp/eastl/` reproducible, staged, and
actionable so generic parser, sema, and HIR fixes can land one mechanism family
at a time before broader container follow-up.

## Core Rule

Use EASTL as pressure for generic compiler behavior. Do not land EASTL-specific
 parsing exceptions or silently expand this runbook into unrelated runtime, ABI,
 or standard-library initiatives.

## Read First

- [ideas/open/47_eastl_container_bringup_plan.md](/workspaces/c4c/ideas/open/47_eastl_container_bringup_plan.md)
- [tests/cpp/eastl/README.md](/workspaces/c4c/tests/cpp/eastl/README.md)
- [prompts/EXECUTE_PLAN.md](/workspaces/c4c/prompts/EXECUTE_PLAN.md)

## Current Targets

- keep the Step 1 and Step 2 EASTL baseline visible and reproducible
- resume Step 3 from the smallest active parser frontier, not from
  `eastl_vector_simple.cpp` by default
- separate parse-only, canonical/sema, HIR, and runtime frontiers per case
- use recipe-backed reproductions whenever possible

## Non-Goals

- reopening raw `std::vector` bring-up as the primary frontier
- solving broad runtime or ABI integration beyond temporary local shims already
  allowed by the source idea
- jumping to new container families before the existing EASTL ladder is stable
- absorbing larger prerequisite initiatives into this runbook without spinning
  them into `ideas/open/`

## Working Model

The active EASTL ladder is:

1. Step 1 foundation headers and traits are complete enough for routine
   reproduction
2. Step 2 object-lifetime and utility coverage is partially mapped, with some
   cases now timing out later in parse or canonical flows
3. Step 3 vector-facing work remains deferred until the smaller tuple and
   memory-frontier parser blockers are reduced further

Use `tests/cpp/eastl/README.md` as the current inventory of earliest failing
stages, and update it when a testcase meaningfully moves.

## Execution Rules

- choose the smallest testcase that exposes the next missing generic mechanism
- keep parser, sema, HIR, and runtime investigations separated
- prefer recipe or test coverage before source changes
- compare against Clang when parser or semantic intent is unclear
- if a newly discovered blocker is larger than the current slice, record it in
  `ideas/open/` instead of mutating this plan

## Step 1: Preserve The EASTL Baseline Matrix

Goal: keep the testcase inventory current enough that another agent can resume
without re-triaging the whole EASTL directory.

Primary targets:

- [tests/cpp/eastl/README.md](/workspaces/c4c/tests/cpp/eastl/README.md)
- `tests/cpp/eastl/*.cpp`
- recipe coverage in `tests/cpp/eastl/*.cmake`

Actions:

- verify the shared include recipe still matches current repro commands
- confirm each EASTL testcase has its earliest failing stage recorded
- add or refresh bounded recipe coverage when a testcase frontier changes

Completion check:

- the current failing stage and one reproducible command are documented for the
  testcase being worked

## Step 2: Work The Simplest Active Frontier

Goal: resume from the narrowest testcase that still exposes the next generic
missing mechanism.

Primary targets:

- [tests/cpp/eastl/eastl_tuple_simple.cpp](/workspaces/c4c/tests/cpp/eastl/eastl_tuple_simple.cpp)
- [tests/cpp/eastl/eastl_memory_simple.cpp](/workspaces/c4c/tests/cpp/eastl/eastl_memory_simple.cpp)
- parser debug output and reduced reproductions derived from those cases

Actions:

- start from the current `eastl_tuple_simple.cpp` timeout path before touching
  `eastl_vector_simple.cpp`
- reduce the active parser blocker until it becomes a bounded generic repro
- if `eastl_memory_simple.cpp` exposes the same root cause more simply, switch
  to it and record the reason in `todo.md`

Completion check:

- the next blocker is reduced to one concrete parser, sema, or HIR mechanism
  with a smallest known reproducer

## Step 3: Fix One Generic Mechanism Family At A Time

Goal: land the narrowest generic compiler fix justified by the active EASTL
reproducer.

Primary targets:

- `src/frontend/parser/`
- `src/frontend/sema/`
- `src/frontend/hir/`

Actions:

- classify the blocker as parser, sema, or HIR before editing code
- add or update the narrowest validating regression first
- implement one root cause at a time and avoid testcase-shaped exceptions

Completion check:

- the targeted EASTL reproducer moves forward and the focused regression covers
  the repaired mechanism

## Step 4: Revalidate The Ladder After Each Slice

Goal: ensure progress on one testcase does not regress earlier EASTL rungs.

Actions:

- rerun the directly affected testcase and nearby earlier EASTL cases
- update `tests/cpp/eastl/README.md` when the earliest failing stage changes
- keep `todo.md` current with the next intended slice and any bounded blockers

Completion check:

- the moved testcase is documented, earlier simpler cases still behave as
  expected, and the next slice is explicit

## Step 5: Return To `eastl::vector` Only When Smaller Cases Stop Leading

Goal: keep vector work bounded and resumable instead of making it the default
catch-all frontier.

Actions:

- treat `eastl_vector_simple.cpp` as the active target only after smaller EASTL
  cases no longer expose the next missing mechanism
- prefer parse-only or reduced canonical workflows before full semantic or
  runtime pressure
- record any separate prerequisite idea if vector progress depends on a broader
  initiative

Completion check:

- vector work is backed by a bounded reproduction and does not hide a simpler
  unresolved upstream blocker
