# AArch64 Calls Emission Consolidation Runbook

Status: Active
Source Idea: ideas/open/02_aarch64_calls_emission_consolidation.md

## Purpose

Consolidate the AArch64 call-emission surface now that shared prepared
call-plan authority exists.

## Goal

Leave AArch64 target-local calls code responsible for emitting machine nodes
from prepared call facts, not for rederiving call-planning decisions.

## Core Rule

Every consolidation step must remove duplicate authority or clarify an
emission-only boundary; do not hide planning behavior in renamed helpers or
weaken tests to make files look smaller.

## Read First

- `ideas/open/02_aarch64_calls_emission_consolidation.md`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `src/backend/mir/aarch64/codegen/calls_preservation.cpp`
- `src/backend/mir/aarch64/codegen/README.md`
- AArch64 backend call tests under `tests/backend/mir/`

## Current Targets / Scope

- AArch64 `calls*.cpp` and `calls*.hpp` call-emission ownership.
- Build metadata that names retired call-emission files.
- Printer or shared MIR placement for call spelling/effect code that is not
  AArch64 emission.
- Tests that prove call emission still consumes prepared call-plan facts.

## Non-Goals

- Do not invent a new shared call-plan API.
- Do not perform broad dispatch cleanup outside call emission.
- Do not change behavior solely to reduce line count.
- Do not perform unrelated ALU, memory, comparison, variadic, prologue, or
  non-call lowering cleanup.
- Do not move AArch64-specific emission details into the shared planner.

## Working Model

- Shared prepared call-plan facts own call-planning decisions.
- AArch64 call code may select target machine forms, registers, stack
  references, and instruction sequences needed to emit those prepared facts.
- Helper files are acceptable only when their boundary is plainly
  emission-only.
- File retirement is valid only when build metadata, include graphs, and tests
  agree with the new ownership boundary.

## Execution Rules

- Start each code-changing step with a narrow audit of the current call files
  and their direct tests.
- Prefer deleting obsolete reconstruction paths over wrapping them in new
  compatibility helpers.
- Keep each slice coherent enough for a fresh build and focused backend tests.
- Escalate to broader backend validation after retiring files or moving shared
  call-printing/effect ownership.
- Reject testcase-shaped shortcuts, expectation downgrades, or pure file
  concatenation.

## Step 1: Audit AArch64 Call Emission Ownership

Goal: identify the first obsolete AArch64 call-emission shard or helper family
that can be safely consolidated.

Primary targets:

- `src/backend/mir/aarch64/codegen/calls*.cpp`
- `src/backend/mir/aarch64/codegen/calls*.hpp`
- build metadata that references those files
- focused backend call tests

Actions:

- Inspect each AArch64 call translation unit and classify its logic as
  emission, prepared-fact consumption, duplicate planning reconstruction,
  printing/effect spelling, or unrelated glue.
- Identify any code paths that still reconstruct argument placement,
  preservation, byval, or call-boundary details already present in prepared
  call-plan facts.
- Pick the first consolidation family with a concrete deletion or move path and
  matching focused proof command.
- Record the selected family and proof scope in `todo.md`; do not edit source
  idea text for routine audit findings.

Completion check:

- `todo.md` names the chosen first consolidation family, the relevant files,
  and the focused build/test proof command.

## Step 2: Remove the First Duplicate Planning Path

Goal: replace the chosen AArch64-local reconstruction path with direct
prepared-fact consumption or delete it if it is now unused.

Actions:

- Make the smallest code change that removes duplicate call-planning authority
  from AArch64-local code.
- Keep target-local logic limited to emission choices.
- Update focused tests or add coverage only to preserve the stronger ownership
  contract; do not weaken expectations.
- Run `cmake --build --preset default` plus the focused backend call tests
  selected in Step 1.

Completion check:

- The duplicate path is gone or no longer owns the decision, and the focused
  proof passes with `test_after.log` updated.

## Step 3: Retire or Merge Emission-Only Shards

Goal: reduce the `calls*.cpp` surface where the first consolidation makes a
separate file or helper boundary unnecessary.

Actions:

- Merge small emission-only helpers into the remaining call owner when that
  improves responsibility clarity.
- Delete retired source/header files and update build metadata.
- Keep optional helper files only when their name and interface describe a real
  emission-only boundary.
- Run a fresh build and focused backend call tests after each coherent file
  retirement.

Completion check:

- Retired files are removed from build metadata and include graphs, and the
  remaining call files have clearer responsibilities.

## Step 4: Rehome Non-Emission Call Spelling or Effects

Goal: move call printing or effect spelling out of AArch64 call emission when
it belongs in machine-printer or shared MIR ownership.

Actions:

- Identify any call spelling/effect logic that does not select AArch64 machine
  emission.
- Move it to the appropriate existing printer or shared MIR layer without
  introducing a new planner API.
- Add or update tests that prove behavior is preserved through the existing
  compiled machine-node route.
- Run build, focused backend call tests, and a broader backend subset when
  ownership moves across module boundaries.

Completion check:

- AArch64 call emission no longer owns non-emission spelling/effect behavior,
  and broader backend proof passes for the moved boundary.

## Step 5: Consolidation Review and Close Decision

Goal: decide whether the source idea is complete or whether another runbook
checkpoint is needed.

Actions:

- Review the final `calls*.cpp`/`calls*.hpp` file set against the source idea
  acceptance criteria.
- Confirm AArch64 call code no longer rederives call-plan decisions already
  present in shared prepared facts.
- Confirm any remaining helper files have explicit emission-only ownership.
- Run the supervisor-selected broader backend validation scope before closure.
- If durable remaining work exists, leave it in `todo.md` or request a plan
  rewrite instead of closing the idea.

Completion check:

- The supervisor has enough evidence to close the idea or request a narrower
  follow-up plan without ambiguity.
