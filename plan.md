# AArch64 Memory Post-Contract Boundary Audit Runbook

Status: Active
Source Idea: ideas/open/124_aarch64_memory_post_contract_boundary_audit.md

## Purpose

Audit `src/backend/mir/aarch64/codegen/memory.cpp` after recent prepared,
publication, calls, dispatch, and comparison contract work. Produce focused
follow-up ideas only for concrete remaining memory-side boundary gaps.

## Goal

Classify whether `memory.cpp` still owns shared prepared/BIR authority or is
now only target-local AArch64 memory emission, then close with evidence and
follow-up ideas where needed.

## Core Rule

This route is analysis-only. Do not edit implementation files, tests, build
metadata, or `ideas/closed/*`.

## Read First

- `ideas/open/124_aarch64_memory_post_contract_boundary_audit.md`
- Closed idea context for ideas 34, 39, 39a, 50, 70, 86, 88, 89, 111, 114,
  116, 117, 122, and 123
- `src/backend/mir/aarch64/codegen/memory.cpp`
- Nearby declarations only as needed to understand `memory.cpp` helper
  ownership and call paths

## Current Targets

Audit these five standards:

1. Prepared memory record and identity validation
2. Store-source and store-global publication consumption
3. Frame-slot and pointer-base materialization residue
4. Variadic `va_list` field memory handling
5. Physical split readiness

## Non-Goals

- Do not implement repairs in `memory.cpp`, `memory.hpp`, tests, or build
  metadata.
- Do not reopen closed routes without new evidence.
- Do not move AArch64-specific addressing legality, scratch/base register
  policy, load/store opcode choice, ABI-specific `va_list` field layout, or
  machine-record emission into shared code.
- Do not treat line-count reduction or helper movement as semantic progress.
- Do not start x86 or RISC-V backend work.

## Working Model

- Shared prepared/BIR authority includes target-neutral identity,
  publication, materialization, and consistency facts.
- AArch64 memory emission includes operand legality, addressing modes,
  scratch/base register policy, opcode choice, diagnostic spelling, ABI
  layout, and machine-record emission.
- A clean boundary may still justify a local owner split for private clarity,
  but that split is not semantic progress.

## Execution Rules

- Keep findings tied to concrete functions, helpers, files, and closed-route
  evidence.
- For each audit standard, record either a concrete unresolved gap or an
  explicit `no new idea` result with supporting evidence.
- Any generated follow-up idea must name the owner boundary, likely files,
  proof route, and testcase-overfit reject signals.
- Reject vague follow-ups, monolithic shrink plans, expectation downgrades, and
  named-case-only fixes.

## Ordered Steps

### Step 1: Build the evidence inventory

Goal: Establish the current helper/function map and closed-route context before
classifying ownership.

Primary target: `src/backend/mir/aarch64/codegen/memory.cpp`

Actions:

- Inspect the current top-level helper/function clusters in `memory.cpp`.
- Read only the closed idea notes needed to identify already-covered authority
  work.
- Map relevant helpers to the five audit standards.
- Keep the output as audit notes in `todo.md`; do not create follow-up ideas
  yet.

Completion check:

- `todo.md` contains a function/helper cluster map and cites the closed-route
  evidence needed for the next classification step.

### Step 2: Classify the five boundary standards

Goal: Decide which standards are clean, which expose unresolved shared
authority, and which only need local clarity.

Primary target: audit notes from Step 1 plus `memory.cpp`

Actions:

- For each standard, separate shared prepared/BIR authority from AArch64
  emission responsibility.
- Mark standards already covered by closed work as `no new idea` with evidence.
- Identify any unresolved gap with concrete helper names and likely owner.
- Do not infer a gap from size alone.

Completion check:

- `todo.md` contains a classification table for all five standards, including
  `no new idea` entries where current evidence is sufficient.

### Step 3: Draft concrete follow-up ideas if gaps remain

Goal: Convert only proven unresolved gaps into durable open ideas.

Primary target: `ideas/open/`

Actions:

- Create one focused `ideas/open/*.md` follow-up per distinct unresolved
  boundary gap, if any.
- Include owner boundary, likely files, proof route, and testcase-overfit reject
  signals in every follow-up.
- Do not duplicate ideas 34, 39, 39a, 50, 70, 86, 88, 89, 111, 114, 116, 117,
  122, or 123.
- If no gaps remain, do not create a follow-up idea.

Completion check:

- Every generated follow-up is concrete and non-duplicative, or the audit notes
  explicitly record that no follow-up idea is needed.

### Step 4: Prepare closure evidence

Goal: Make the source idea closure decision reviewable without implementation
changes.

Primary target: `todo.md` and, if closure is accepted, the source idea

Actions:

- Summarize the five-standard classification table.
- Summarize the current `memory.cpp` helper/function cluster map.
- List generated follow-up ideas or explicit `no new idea` decisions.
- Ask the plan owner lifecycle close path to decide whether the source idea is
  complete and to move it only after the close gate is satisfied.

Completion check:

- The closure note distinguishes shared prepared/BIR authority from AArch64
  memory emission for each standard and names any remaining open follow-ups.
