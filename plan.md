# Full X86 Backend Contract-First Replan

Status: Active
Source Idea: ideas/open/86_full_x86_backend_contract_first_replan.md
Supersedes: ideas/open/82_extract_full_x86_backend_subsystem_to_markdown_for_phoenix_rebuild.md
Activated from: ideas/open/86_full_x86_backend_contract_first_replan.md

## Purpose

Rebuild the active `src/backend/mir/x86/` route around explicit interface
contracts before further behavior recovery. This runbook treats markdown as
formal design contract, keeps compatibility seams visibly thin, and isolates
known BIR gaps into later work instead of smearing workaround policy across the
x86 tree.

## Goal

Make the x86 backend tree readable, compilable, and ownership-driven:

- root `x86/` describes the whole subsystem honestly
- `x86/` has stable subsystem seams with live headers/cpps
- key in-place `*.cpp.md` / `*.hpp.md` record both legacy evidence and new
  design contract
- future behavior packets can point at contract sections instead of inventing
  boundaries while coding

## Core Rules

- prefer README and contract markdown to define ownership first
- keep `backend.cpp` depending only on thin x86 public surfaces
- keep `assembler/` and `linker/` as explicit subsystem contracts even when
  live implementation is deferred
- do not use x86 local patches to hide upstream BIR capability gaps
- do not accept behavior-recovery work that cannot point to an existing
  contract section

## Step 1: Backend/X86 Architecture Contract Pass

Goal: make `src/backend/` and `src/backend/mir/x86/` the canonical
architecture contracts for the shared backend pipeline and the x86 ownership
graph.

Primary targets:

- `src/backend/README.md`
- `src/backend/mir/x86/README.md`
- `src/backend/mir/x86/assembler/README.md`
- `src/backend/mir/x86/linker/README.md`

Completion check:

- every x86 subsystem has an explicit ownership statement
- README terminology matches the live tree
- deferred subsystems are marked as deferred, not silently absent

## Step 2: Subsystem Contract Completion Pass

Goal: promote key in-place markdown companions into formal contract documents.

Primary targets:

- public or compatibility headers under `src/backend/mir/x86/`
- key subsystem entrypoints in `abi/`, `module/`, `debug/`, and `lowering/`

Completion check:

- selected `*.cpp.md` / `*.hpp.md` use fixed sections for `Legacy Evidence`
  and `Design Contract`
- each design contract states inputs, outputs, ownership, and deferred gaps

## Step 3: Live Interface Conformance Pass

Goal: ensure the live `x86/` tree matches the new ownership graph and
compiles through thin seams.

Primary targets:

- `src/backend/mir/x86/api/`
- `src/backend/mir/x86/core/`
- `src/backend/mir/x86/abi/`
- `src/backend/mir/x86/module/`
- `src/backend/mir/x86/lowering/`
- `src/backend/mir/x86/prepared/`
- `src/backend/mir/x86/debug/`

Completion check:

- `backend.cpp` includes only approved x86 public headers
- the x86 subsystem subdirectories exist as real live seams
- `c4c_backend` builds with the contract-first skeleton in place

## Step 4: Behavior Recovery Packets

Goal: refill real x86 behavior behind the contract-defined seams without
reopening architectural ambiguity.

Primary targets:

- function/module emit bodies
- ABI/lowering policy
- prepared fast-path behavior
- debug route fidelity

Completion check:

- each packet points to the relevant contract section first
- no packet mixes new ownership decisions with behavior-only repair
