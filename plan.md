# x86 Prepared Edge Publication Coverage Broadening Runbook

Status: Active
Source Idea: ideas/open/22_x86_prepared_edge_publication_coverage_broadening.md
Activated from: ideas/closed/21_x86_prepared_edge_publication_consumer.md follow-up

## Purpose

Broaden x86 prepared edge-publication lowering coverage beyond the first
joined-branch consumer from idea 21 while preserving shared prepared lookup
authority.

## Goal

Make more than one nearby x86 edge-publication or block-entry lowering path
consume `x86::ConsumedPlans::shared_function_lookups()->edge_publications`
without reintroducing x86-local semantic rediscovery.

## Core Rule

Shared prepare owns semantic publication facts and lookup indexing. x86 owns
scratch selection, clobber policy, physical register spelling, register-class
constraints, stack operand syntax, move spelling, control-flow emission, and
final assembly formatting.

## Read First

- `ideas/open/22_x86_prepared_edge_publication_coverage_broadening.md`
- `ideas/closed/21_x86_prepared_edge_publication_consumer.md`
- `src/backend/mir/x86/prepared/prepared.hpp`
- `src/backend/mir/x86/prepared/dispatch.cpp`
- `src/backend/mir/x86/core/core.cpp`
- `src/backend/mir/x86/module/module.cpp`
- `tests/backend/bir/backend_x86_handoff_boundary_joined_branch_test.cpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`

## Current Scope

- Extend x86 consumption of prepared edge-publication facts to additional
  nearby edge-publication or block-entry paths.
- Cover at least one source/destination home combination or edge shape not
  covered by the idea 21 joined-branch proof.
- Add focused tests that would fail if x86 ignored shared `edge_publications`
  or rebuilt edge-copy facts locally.
- Preserve AArch64 behavior and the shared prepared lookup contract.

## Non-Goals

- Do not implement full x86 codegen.
- Do not implement or claim RISC-V consumer readiness.
- Do not broadly rewrite x86 control-flow lowering.
- Do not add an x86-local edge-copy table, predecessor/successor rediscovery
  pass, or fallback semantic authority.
- Do not move x86 register spelling, scratch policy, stack syntax, move
  spelling, or assembly formatting into shared prepare, BIR, or target-neutral
  helpers.
- Do not weaken supported-path expectations or accept expectation-only
  progress.

## Working Model

- Existing shared lookup entry point:
  `x86::ConsumedPlans::shared_function_lookups()->edge_publications`.
- Existing x86 consumer helper:
  `x86::prepared::consume_edge_publication_move_intent`.
- Existing supported movement shape from idea 21:
  stack-slot source to register destination for an available prepared move.
- New work should first identify the nearest extension point around that
  helper, then add one coherent generalization rather than matching a single
  named testcase.

## Execution Rules

- Keep changes small enough for one executor packet to prove with a focused
  backend subset.
- Prefer semantic support for a home or edge family over testcase-shaped
  branches.
- Keep missing-authority and unsupported-home statuses explicit and tested.
- When a target path is intentionally unsupported, prove it fails closed
  without local rediscovery.
- If a required extension is a separate initiative, stop and ask the supervisor
  for lifecycle routing instead of expanding this plan.

## Ordered Steps

### Step 1: Map Current Consumer Surface

Goal: Identify the smallest nearby x86 coverage gap that can be broadened
without a broad backend rewrite.

Primary targets:
- `src/backend/mir/x86/prepared/dispatch.cpp`
- `src/backend/mir/x86/prepared/prepared.hpp`
- `src/backend/mir/x86/core/core.cpp`
- `tests/backend/bir/backend_x86_handoff_boundary_joined_branch_test.cpp`

Concrete actions:
- Inspect where `append_edge_publication_move_instruction` is called and which
  predecessor/successor/destination facts it receives.
- List the current supported and unsupported home combinations in
  `consume_edge_publication_move_intent`.
- Pick one nearby broadening target: another edge-publication path,
  block-entry path, or one additional source/destination home family that the
  current x86 lowering surface can emit.
- Record the chosen executor packet in `todo.md` before implementation.

Completion check:
- The executor can state the chosen broadening target and why it is not a
  named-case shortcut.

### Step 2: Broaden x86 Prepared Consumption

Goal: Implement the selected broadening target using shared prepared
edge-publication facts.

Primary targets:
- `src/backend/mir/x86/prepared/dispatch.cpp`
- `src/backend/mir/x86/prepared/prepared.hpp`
- x86 lowering/module files only as required by the selected path

Concrete actions:
- Extend the existing prepared consumer or add a narrowly named x86-prepared
  helper that still starts from shared `edge_publications`.
- Emit only x86-owned target details from x86 code.
- Preserve explicit statuses for missing shared lookups, missing publication,
  unsupported publication, and unsupported homes.
- Avoid scanning BIR predecessor/successor edges to rediscover publication
  semantics.

Completion check:
- More than the original idea 21 path can emit or classify an x86 move intent
  from the shared edge-publication lookup.

### Step 3: Add Focused Authority Tests

Goal: Prove the broadened x86 path depends on shared prepared authority.

Primary targets:
- `tests/backend/bir/backend_x86_handoff_boundary_joined_branch_test.cpp`
- nearby x86 handoff or prepared lookup tests when a new focused test is
  cleaner
- `tests/backend/bir/CMakeLists.txt` only if a new test binary is needed

Concrete actions:
- Add at least one positive test for the newly covered edge/home shape.
- Add or preserve a negative/mutation check that fails when shared
  `edge_publications` are absent, ignored, or replaced with local rediscovery.
- Keep unsupported-home behavior explicit when the selected path has adjacent
  homes that remain unsupported.

Completion check:
- The focused tests distinguish shared lookup consumption from local semantic
  reconstruction.

### Step 4: Validate and Handoff

Goal: Prove the slice at the right scope and record whether x86 is ready for a
separate RISC-V consumer idea.

Primary targets:
- build target for the touched backend tests
- focused CTest names for x86 handoff/prepared coverage
- broader backend bucket selected by the supervisor

Concrete actions:
- Run the supervisor-delegated build and focused test command exactly.
- Run the broader backend validation command if delegated for this slice.
- Update `todo.md` proof with commands and outcomes.
- State whether x86 coverage is broad enough for a future RISC-V consumer idea
  or whether more x86 edge/home combinations remain first.

Completion check:
- Validation covers the broadened x86 behavior, shared prepared lookup surface,
  and the relevant backend bucket; `todo.md` contains the proof and handoff.
