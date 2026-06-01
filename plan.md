# AArch64 Large Owner Residue Audit Runbook

Status: Active
Source Idea: ideas/open/68_aarch64_large_owner_residue_audit.md

## Purpose

Audit the largest remaining AArch64 codegen owner files after calls and
dispatch-family cleanup, classify what should stay target-local versus what
should consume or motivate shared backend authority, and prepare coherent
follow-up cleanup ideas.

## Goal

Produce a durable classification table and scoped follow-up ideas before any
implementation cleanup in the large AArch64 owner files.

## Core Rule

This plan is audit-only. Do not edit `src/backend/mir/aarch64/codegen/*.cpp`,
`src/backend/mir/aarch64/codegen/*.hpp`, or any implementation file while
claiming progress on this runbook.

## Read First

- `ideas/open/68_aarch64_large_owner_residue_audit.md`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- reference ARM codegen layout under `src/backend/mir/arm/`
- shared backend and prealloc surfaces that already provide BIR, prepared,
  or allocation facts

## Current Targets

- Inventory the largest AArch64 codegen owners by line count and responsibility.
- Compare each high-residue owner against the ARM codegen layout and shared
  backend/prealloc responsibilities.
- Classify helper groups as `target-emission`, `consume-shared`,
  `missing-shared-authority`, `fold-back-or-split`, or `needs-more-evidence`.
- Draft numbered follow-up ideas under `ideas/open/` only after the audit has
  enough evidence to scope each cleanup coherently.
- Preserve a final close note explaining why dispatch-family cleanup is not the
  primary next route.

## Non-Goals

- Do not implement cleanup in `calls.cpp`, `memory.cpp`, `alu.cpp`,
  `machine_printer.cpp`, or `instruction.*`.
- Do not reopen dispatch-family contraction unless the audit finds a concrete
  cross-owner dependency.
- Do not treat line-count reduction as success without confirming authority
  ownership.
- Do not merge or split files mechanically without naming the semantic residue.
- Do not weaken tests, diagnostics, supported behavior, or reviewer contracts.

## Working Model

- `target-emission`: keep local because the code performs AArch64 instruction
  selection, register/scratch selection, addressing, ABI sequencing,
  instruction records, or printer spelling.
- `consume-shared`: an existing BIR, prealloc, prepared, or shared backend fact
  should be consumed more directly.
- `missing-shared-authority`: cleanup needs a new shared fact/query before the
  AArch64 owner can shrink safely.
- `fold-back-or-split`: code belongs in a more precise AArch64 owner or helper.
- `needs-more-evidence`: the audit needs a narrow probe before deciding.

## Execution Rules

- Keep routine audit notes in `todo.md` until they are ready to become durable
  classification or follow-up idea text.
- If creating follow-up ideas, keep each idea scoped by one owner family or one
  shared-authority family. Do not create one broad "shrink AArch64" idea.
- Name concrete helpers, facts, owners, or duplicated authority in each finding.
- Treat machine records and printer spelling as target-local unless evidence
  shows semantic duplication.
- Any build or test command is optional for pure audit text changes, but
  follow-up ideas must name the proof shape their future implementation needs.

## Ordered Steps

### Step 1: Inventory High-Residue Owners

Goal: Establish the current large-owner map before classifying residue.

Primary target:
- `src/backend/mir/aarch64/codegen/`

Concrete actions:
- Measure line counts for the current AArch64 codegen owners.
- Confirm whether the source idea's high-residue list is still complete:
  `calls.cpp`, `memory.cpp`, `alu.cpp`, `machine_printer.cpp`,
  `instruction.cpp`, and `instruction.hpp`.
- Record each owner with a short responsibility summary in `todo.md`.
- Note any additional owner that is now comparable in size or responsibility.

Completion check:
- `todo.md` contains a current owner inventory with line counts and one-line
  responsibilities for every audited file.

### Step 2: Compare Against Reference And Shared Authority

Goal: Separate target-local AArch64 emission from duplicated or missing shared
authority candidates.

Primary targets:
- AArch64 owners from Step 1
- reference ARM codegen layout
- shared backend/prealloc/BIR prepared-fact surfaces

Concrete actions:
- For each AArch64 owner, identify comparable ARM codegen responsibilities.
- Identify helper groups that duplicate existing shared facts or prepared
  authority.
- Identify helper groups that cannot move yet because a shared fact/query is
  missing.
- Keep the comparison at helper-group granularity, not whole-file granularity.

Completion check:
- `todo.md` contains per-owner comparison notes that name the relevant helper
  groups and their likely authority source.

### Step 3: Classify Each Major Helper Group

Goal: Produce the durable audit classification table.

Primary target:
- classification table in `todo.md`, later preserved through follow-up ideas
  and close notes

Concrete actions:
- Classify each major helper group as `target-emission`, `consume-shared`,
  `missing-shared-authority`, `fold-back-or-split`, or `needs-more-evidence`.
- For each `consume-shared` entry, name the existing shared fact or query.
- For each `missing-shared-authority` entry, name the missing fact/query shape.
- For each `fold-back-or-split` entry, name the better local owner boundary.
- For each `needs-more-evidence` entry, define the narrow probe needed.

Completion check:
- A classification table exists with one row per major helper group and a clear
  next action for every audited owner.

### Step 4: Draft Follow-Up Cleanup Ideas

Goal: Convert the classification into coherent next-route ideas without mixing
unrelated owner families.

Primary target:
- new `ideas/open/*.md` follow-up drafts, when delegated through lifecycle
  ownership

Concrete actions:
- Group follow-ups by owner family or shared-authority family.
- For each follow-up, state whether it is mechanical target cleanup,
  shared-authority migration, or an evidence probe.
- Include owned files, explicit non-goals, proof expectations, and reviewer
  reject signals.
- Reject any follow-up that mixes calls, memory, ALU, instruction records, and
  printer changes into one monolithic cleanup.

Completion check:
- Numbered follow-up ideas are ready or created under `ideas/open/`, each with
  named file families and a concrete cleanup route.

### Step 5: Close The Audit Route

Goal: Make the audit result durable and leave the next implementation route
unambiguous.

Primary target:
- the linked source idea close note and lifecycle state

Concrete actions:
- Summarize the final classification and follow-up idea list.
- Explain why dispatch-family cleanup is not being reopened as the primary
  next route, unless a concrete dependency was discovered.
- Confirm that no implementation files changed as part of the audit.
- Ask the lifecycle owner to close the source idea only when the classification
  and follow-up idea criteria are satisfied.

Completion check:
- The source idea can be closed as audit-complete, with durable next actions
  represented by follow-up ideas.
