# EASTL Container Bring-Up Runbook

Status: Active
Source Idea: ideas/open/eastl_container_bringup_plan.md
Activated from: prompts/AGENT_PROMPT_ACTIVATE_PLAN.md

## Purpose

Make `tests/cpp/eastl/*` the active library bring-up frontier, starting with the
smallest reusable EASTL support cases and only moving to heavier container
pressure after the earlier ladder is stable and reproducible.

## Goal

Establish a staged, test-driven EASTL bring-up path that produces generic
parser, sema, HIR, and harness improvements without relying on EASTL-specific
acceptance hacks.

## Core Rule

Treat EASTL as pressure, not as a dialect: land generic frontend fixes whenever
the underlying issue is generic, and only tolerate narrow workarounds for
runtime or ABI support that is explicitly out of scope for this bring-up track.

## Read First

- [`prompts/AGENT_PROMPT_EXECUTE_PLAN.md`](/workspaces/c4c/prompts/AGENT_PROMPT_EXECUTE_PLAN.md)
- [`ideas/open/eastl_container_bringup_plan.md`](/workspaces/c4c/ideas/open/eastl_container_bringup_plan.md)
- [`tests/cpp/eastl`](/workspaces/c4c/tests/cpp/eastl)
- existing EASTL recipes under [`tests/cpp`](/workspaces/c4c/tests/cpp)

## Scope

- turn selected EASTL testcases into stable, reproducible recipes
- classify each testcase by earliest failing stage
- fix the smallest generic blockers needed to advance the current stage
- progress in this order:
  1. foundation headers and traits
  2. object lifetime / utility layer
  3. `eastl::vector` parse frontier
  4. `eastl::vector` sema / HIR / lowering viability
  5. follow-on EASTL containers only after the earlier ladder is healthy

## Non-Goals

- broad STL or libstdc++ bring-up outside the selected EASTL path
- header-name special cases or testcase-shaped EASTL parsing exceptions
- full runtime or ABI ownership for `new`/`delete`, libc++abi, libunwind,
  `virtual`, RTTI, `typeid`, or dynamic-cast-style support
- expanding into additional container families before the current ladder is
  stable

## Working Model

- For each testcase, first make the invocation reproducible.
- Validate `--parse-only` before later pipeline stages.
- Use `--dump-canonical` after parse succeeds.
- Use `--dump-hir-summary` only after semantic validation progresses.
- Only then push full compile or runtime workflows.
- Compare behavior against Clang whenever frontend or codegen behavior is in
  question.

## Execution Rules

- Work from the highest-priority incomplete item in [`plan_todo.md`](/workspaces/c4c/plan_todo.md).
- Add or update the narrowest validating test before implementing a fix.
- Record the full-suite baseline before meaningful implementation changes.
- Keep slices small enough to prove one root cause at a time.
- If a blocker is larger or belongs to a different subsystem, spin it out into
  `ideas/open/` instead of stretching this plan.
- Preserve resumable execution notes in [`plan_todo.md`](/workspaces/c4c/plan_todo.md) whenever the active slice changes.

## Ordered Steps

### Step 1: Inventory and recipe normalization

Goal: make the EASTL cases reproducible and classify their current earliest
failing stage.

Primary targets:
- [`tests/cpp/eastl`](/workspaces/c4c/tests/cpp/eastl)
- existing EASTL workflow recipes under [`tests/cpp`](/workspaces/c4c/tests/cpp)

Actions:
- enumerate the current EASTL testcase set and their required include flags
- define one bounded command per testcase
- record whether each case currently lands in parse, sema, HIR, codegen, or
  runtime failure
- tighten or add recipe coverage where the current workflow is missing

Completion check:
- the selected EASTL cases have reproducible commands and a recorded earliest
  failing stage

### Step 2: Foundation headers and traits

Goal: stabilize the smallest high-leverage EASTL support cases first.

Primary targets:
- `eastl_piecewise_construct_simple.cpp`
- `eastl_tuple_fwd_decls_simple.cpp`
- `eastl_integer_sequence_simple.cpp`
- `eastl_type_traits_simple.cpp`
- `eastl_utility_simple.cpp`

Actions:
- drive each case through parse-only first
- fix the smallest generic parser or semantic blockers exposed by these files
- add or refine targeted tests that isolate the root cause outside the full
  header stack when possible

Completion check:
- the foundation cases are reproducible and their current failures are either
  resolved or narrowed to explicit later-stage blockers

### Step 3: Object lifetime and tuple layer

Goal: advance the next tier of reusable support features before container work.

Primary targets:
- `eastl_memory_simple.cpp`
- `eastl_tuple_simple.cpp`

Actions:
- validate parse behavior, then semantic and HIR progress
- compare against Clang when placement-new, alignment, or tuple patterns are
  unclear
- keep fixes generic unless the issue is explicitly runtime-facing

Completion check:
- memory and tuple cases advance as far as the current frontend supports, with
  blockers clearly classified and minimized

### Step 4: `eastl::vector` parse frontier

Goal: make `eastl_vector_simple.cpp` parse behavior stable and actionable before
deeper lowering work.

Primary targets:
- `eastl_vector_simple.cpp`
- parser-facing recipe support for the vector case

Actions:
- establish a bounded parse-only recipe
- reduce parser failures to the smallest generic missing feature
- avoid mixing parse stabilization with runtime or ABI concerns

Completion check:
- `eastl_vector_simple.cpp` has stable parse-oriented reproduction and its next
  blocking parser issue is either fixed or isolated

### Step 5: `eastl::vector` semantic, HIR, and lowering viability

Goal: advance `eastl::vector` beyond parse only after Step 4 is healthy.

Primary targets:
- semantic, HIR, and lowering support reached by `eastl_vector_simple.cpp`

Actions:
- move one stage at a time: sema, then HIR, then codegen
- permit narrow testcase-local runtime shims only for out-of-scope ABI or
  allocation dependencies
- reject frontend hacks that only recognize EASTL spellings

Completion check:
- the current post-parse `eastl::vector` blocker is removed or isolated with
  proof of stage-by-stage progress

### Step 6: Follow-on container expansion

Goal: extend the same bring-up method to additional EASTL containers only after
the earlier ladder is healthy.

Actions:
- choose the next lightest container-adjacent surface
- reuse the same reproduce, classify, narrow, and validate loop
- spin out a new idea if the new surface becomes a separate initiative

Completion check:
- additional EASTL container work starts from a stable earlier ladder and does
  not overload this runbook
