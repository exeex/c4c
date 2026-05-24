# AArch64 Codegen Forward-Migration Candidate Audit Runbook

Status: Active
Source Idea: ideas/open/aarch64-codegen-forward-migration-candidate-audit.md

## Purpose

Audit `src/backend/mir/aarch64/codegen` and identify responsibilities that should
move earlier into `bir`, `prealloc`, or shared MIR support before x86 and RISC-V
codegen duplicate them.

## Goal

Produce a concise classification of AArch64 codegen responsibilities and turn the
highest-value forward-migration candidates into multiple smaller open ideas.

## Core Rule

This is a discovery and planning runbook. Do not perform broad implementation
movement, weaken tests, or create generic abstractions as part of the audit.

## Read First

- `ideas/open/aarch64-codegen-forward-migration-candidate-audit.md`
- `ref/claudes-c-compiler/src/backend/arm/codegen/README.md`
- `src/backend/mir/aarch64/codegen/README.md`
- `src/backend/prealloc/README.md`
- `src/backend/bir`
- `src/backend/prealloc`
- shared MIR/module code under `src/backend/mir`

## Scope

Inspect all files under `src/backend/mir/aarch64/codegen`, with special focus on:

- `dispatch*.cpp` and `dispatch.hpp`
- non-target-ABI portions of `calls_*.cpp`
- `operands.cpp` and Prepared value-home decoding
- `traversal.cpp` and Prepared index construction
- `compatibility_projection.cpp`
- publication, edge-copy, producer, store-source, and value-materialization
  helpers
- diagnostics and any code interpreting Prepared facts rather than selecting or
  printing AArch64 instructions

Also inspect destination candidates before proposing migration:

- `src/backend/bir`
- `src/backend/prealloc`
- shared MIR/module code under `src/backend/mir`

## Non-Goals

- Do not move AArch64 register spelling, instruction encoding, ABI lane rules,
  or instruction printing into generic layers.
- Do not implement broad code movement during this audit.
- Do not add testcase-shaped shortcuts or expectation downgrades.
- Do not create a generic abstraction unless at least one x86 or RISC-V reuse
  path is plausible and named.
- Do not leave a generated implementation idea so broad that one focused agent
  run cannot complete it.

## Working Model

Classify each notable current AArch64 codegen responsibility into exactly one
primary bucket:

- `Keep target-local`: real AArch64 instruction selection, register spelling,
  target ABI emission, or target printing.
- `Move to prealloc`: target-independent or target-parameterized contract facts
  that should be computed before MIR lowering.
- `Move to BIR`: semantic normalization or IR fact recovery that should be
  expressed before preallocation.
- `Move to shared MIR`: generic Prepared-to-machine-record lowering,
  publication bookkeeping, diagnostics, indexes, or traversal utilities useful
  to x86/RISC-V.
- `Needs target hook`: mostly generic logic that requires a small target policy
  interface instead of full target-local duplication.
- `Unknown / defer`: requires more proof or adjacent target review before
  moving.

## Execution Rules

- Record audit findings before proposing migrations.
- Compare against the reference README inventory explicitly; do not rely on
  intuition about what codegen "should" own.
- Name the concrete current AArch64-owned responsibility for every proposed
  migration idea.
- Name the proposed destination layer and the x86/RISC-V reuse value for every
  proposed migration idea.
- Include proof requirements that prevent testcase overfit in every generated
  idea.
- If a later packet must write `ideas/open/*.md`, route that packet through
  lifecycle authority. Implementation executors should not silently expand this
  plan into source-idea edits.

## Step 1: Inventory AArch64 Codegen Responsibilities

Goal: Build a current responsibility map for `src/backend/mir/aarch64/codegen`.

Primary target: `src/backend/mir/aarch64/codegen`

Actions:

- List all codegen source/header files and group them by responsibility family.
- Mark obvious reference-model codegen responsibilities such as prologue,
  return ABI, calls ABI emission, instruction selection, memory instruction
  emission, ALU/comparison/casts, float/i128/f128/atomics/intrinsics, globals,
  variadic support, inline asm, asm emission, and peephole work.
- Separately mark files or helper groups that interpret Prepared facts,
  construct Prepared indexes, perform publication bookkeeping, classify edge
  copies, decode value homes, or provide generic diagnostics.

Completion check:

- The audit has a file/helper-group inventory that covers all AArch64 codegen
  files and distinguishes target-local emission from Prepared/MIR bridge logic.

## Step 2: Compare Against the Reference Codegen Model

Goal: Identify which current AArch64 responsibilities fall outside the reference
architecture-codegen inventory.

Primary target: `ref/claudes-c-compiler/src/backend/arm/codegen/README.md`

Actions:

- Compare each responsibility family from Step 1 to the reference file
  inventory and architecture overview.
- Call out current `src/backend/mir/aarch64/codegen` files that do not map
  cleanly to the reference model.
- Keep target ABI emission and instruction printing in the target-local bucket
  even when adjacent helper code may be migration-worthy.

Completion check:

- The audit explicitly names which current AArch64 codegen files or helper
  groups are outside the reference responsibility model.

## Step 3: Inspect Candidate Destination Layers

Goal: Avoid proposing migrations into layers that cannot plausibly own the
responsibility.

Primary targets:

- `src/backend/bir`
- `src/backend/prealloc`
- shared MIR/module code under `src/backend/mir`
- early x86/RISC-V codegen surfaces under `src/backend/mir/x86` and
  `src/backend/mir/riscv`

Actions:

- Inspect existing BIR normalization and fact recovery boundaries.
- Inspect existing prealloc Prepared contract, storage, call, value-home,
  out-of-SSA, and stack-layout helpers.
- Inspect shared MIR utilities and target backend shapes for plausible
  cross-target reuse points.
- For each migration candidate, identify whether x86 or RISC-V would consume it
  directly, consume it through a target hook, or need further design first.

Completion check:

- Each candidate has a plausible destination layer or is explicitly classified
  `Unknown / defer`.

## Step 4: Produce the Audit Table and Priority List

Goal: Turn inspection into a reviewable decision artifact before creating
follow-up implementation ideas.

Actions:

- Produce a concise audit table mapping current AArch64 codegen files or helper
  groups to the classification buckets.
- Include the reference-model comparison in the table or in an adjacent summary.
- Produce a prioritized forward-migration list ordered by x86/RISC-V reuse value
  and implementation risk.
- Include at least these candidate directions when supported by evidence:
  Prepared move/publication indexing, value-home and storage-encoding
  interpretation, entry-formal publication planning, call boundary move
  classification, edge-copy and block-entry publication bookkeeping, and shared
  diagnostics for missing Prepared facts.

Completion check:

- The audit table, reference comparison, and prioritized candidate list are
  ready for supervisor review and can be used to draft concrete open ideas.

## Step 5: Generate Focused Follow-Up Ideas

Goal: Convert the accepted priority list into multiple small source ideas under
`ideas/open/`.

Primary target: `ideas/open/`

Actions:

- For each accepted migration candidate, draft one focused source idea.
- Each idea must name:
  - the current AArch64-owned responsibility
  - the proposed destination layer
  - why x86/RISC-V would benefit
  - proof needed to avoid testcase overfit
  - concrete reviewer reject signals
- Keep each idea small enough for one focused agent run.
- Do not activate a generated idea unless the supervisor or plan owner
  intentionally switches lifecycle state.

Completion check:

- Multiple concrete `ideas/open/*.md` files exist for the highest-value
  migration candidates, and no implementation plan is left half-open.

## Step 6: Completion Review

Goal: Decide whether the source idea is satisfied or whether another audit pass
is needed.

Actions:

- Confirm the audit identifies which AArch64 codegen responsibilities are
  outside the reference codegen model.
- Confirm multiple concrete follow-up ideas exist under `ideas/open/`.
- Confirm no implementation changes or weakened contracts were used as audit
  evidence.
- If the source idea is complete, route closure through the plan owner. If not,
  keep remaining audit gaps in `todo.md` rather than rewriting the source idea.

Completion check:

- The active runbook can be closed or explicitly extended without confusing
  runbook exhaustion with source-idea completion.
