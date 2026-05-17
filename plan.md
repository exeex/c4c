# AArch64 F128 Shard Redistribution Runbook

Status: Active
Source Idea: ideas/open/261_aarch64_f128_markdown_shard_implementation_redistribution.md

## Purpose

Move the useful AArch64 binary128/f128 shard behavior out of broad codegen
owners and into compiled `f128.cpp` / `f128.hpp` owners without changing
semantics.

## Goal

`src/backend/mir/aarch64/codegen/f128.cpp` and `f128.hpp` should own valid
f128 construction, lowering, transport, helper-call, and spelling behavior
that is currently described by `f128.md` or stranded in family-neutral files.

## Core Rule

This is behavior-preserving ownership redistribution. Do not expand f128
semantics, weaken tests, convert deferred/fail-closed boundaries into new live
behavior, or redesign the instruction type hierarchy just to split this shard.

## Read First

- `ideas/open/261_aarch64_f128_markdown_shard_implementation_redistribution.md`
- `src/backend/mir/aarch64/codegen/f128.md`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `src/backend/mir/aarch64/codegen/CMakeLists.txt` or the nearest build file
  that lists AArch64 codegen sources, if present

## Current Scope

- Create compiled `src/backend/mir/aarch64/codegen/f128.cpp` and
  `src/backend/mir/aarch64/codegen/f128.hpp` owners.
- Move f128-specific selected-node construction, lowering, helper-call
  transport, source tracking hooks, and spelling helpers from broad owners into
  the f128 owner where behavior already exists.
- Keep `instruction.cpp` focused on family-neutral instruction core.
- Keep `dispatch.cpp` as routing into f128-owned lowering rather than a place
  that contains f128-family bodies.
- Keep `machine_printer.cpp` from retaining f128-specific spelling bodies when
  those can be owned by f128 helpers.
- Delete `src/backend/mir/aarch64/codegen/f128.md` only after its durable valid
  content has been reconciled into compiled owners.

## Non-Goals

- Do not redistribute any other AArch64 codegen markdown shard.
- Do not redistribute `mod.md` or `records.md`.
- Do not perform broad AArch64 codegen cleanup unrelated to the f128 shard.
- Do not expand binary128/f128 semantics beyond behavior-preserving
  relocation.
- Do not convert fail-closed or deferred-helper f128 boundaries into new live
  behavior.
- Do not restore a legacy centralized record pile under a new name.
- Do not rewrite tests or expectations to make movement appear complete.
- Do not redesign `instruction.hpp` or the instruction type hierarchy solely
  for this shard.

## Working Model

The f128 shard should become an ordinary compiled family owner. Broad codegen
files may keep neutral types, shared instruction machinery, and routing calls,
but f128-specific construction, lowering, transport, helper-call bridges,
source tracking, and spelling belong behind f128-named helpers.

## Execution Rules

- Start by classifying existing f128 behavior before moving code.
- Preserve exact emitted behavior and fail-closed boundaries unless a separate
  approved source idea changes semantics.
- Prefer small extraction packets that can be proven with build plus focused
  backend/AArch64 tests.
- Keep build-file changes limited to registering the new f128 compiled owner.
- Treat expectation downgrades, named-case shortcuts, helper renames without
  ownership movement, and testcase-shaped spellings as route failures.
- Keep routine packet progress in `todo.md`; rewrite this runbook only for a
  real route correction.

## Ordered Steps

### Step 1: Audit F128 Ownership And Build Surface

Goal: identify all current f128-specific code and the build integration needed
for compiled owners before moving behavior.

Primary targets:

- `src/backend/mir/aarch64/codegen/f128.md`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- AArch64 codegen build-source lists

Concrete actions:

- Classify f128-selected-node construction, helper-call transport, source
  tracking, lowering, and spelling currently owned by broad files.
- Identify fail-closed or deferred-helper boundaries that must remain
  behavior-preserving during relocation.
- Identify the minimal build-file update required for `f128.cpp`.
- Record packet notes in `todo.md` rather than editing the source idea.

Completion check:

- A later executor can name the f128 behavior to move, the files it should
  leave, the new owner boundary, and the proof command for the first code
  extraction.

### Step 2: Create The F128 Compiled Owner Boundary

Goal: add `f128.cpp` and `f128.hpp` as build-integrated owners without changing
behavior.

Primary targets:

- `src/backend/mir/aarch64/codegen/f128.cpp`
- `src/backend/mir/aarch64/codegen/f128.hpp`
- AArch64 codegen build-source lists
- Nearby broad owners that will include or call the new header

Concrete actions:

- Create the f128 header/source using the existing AArch64 codegen namespace
  and include style.
- Register `f128.cpp` in the same build mechanism as sibling compiled shards.
- Add only behavior-neutral declarations or forwarding helpers needed to make
  later movement incremental.
- Keep `instruction.hpp` dependency changes minimal; each shard source may
  include the existing instruction header.

Completion check:

- The project builds with the new f128 files present and no f128 semantics have
  changed.

### Step 3: Move F128 Construction, Transport, And Lowering Bodies

Goal: relocate valid f128-family implementation from broad owners into the
compiled f128 owner.

Primary targets:

- `src/backend/mir/aarch64/codegen/f128.cpp`
- `src/backend/mir/aarch64/codegen/f128.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`

Concrete actions:

- Move f128-specific selected-node construction and helper transport behind
  f128-named helpers.
- Route dispatch through the f128 owner instead of keeping f128-family bodies
  in `dispatch.cpp`.
- Preserve full 16-byte binary128 transport, Q-register conventions, helper
  call boundaries, and source-tracking semantics described by `f128.md`.
- Preserve any existing fail-closed or deferred-helper behavior rather than
  using this step to add new semantics.

Completion check:

- `instruction.cpp` and `dispatch.cpp` retain neutral instruction machinery and
  routing only, while focused build/backend proof shows behavior is preserved.

### Step 4: Move F128 Spelling And Printer Helpers

Goal: keep f128-specific spelling out of generic machine printing where the
spelling belongs to the f128 family.

Primary targets:

- `src/backend/mir/aarch64/codegen/f128.cpp`
- `src/backend/mir/aarch64/codegen/f128.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.hpp`

Concrete actions:

- Move f128-specific instruction or operand spelling helpers into the f128
  owner when the spelling is family-specific.
- Keep generic printer traversal and family-neutral formatting in
  `machine_printer.cpp`.
- Preserve exact assembly text for existing supported paths.

Completion check:

- `machine_printer.cpp` no longer owns f128-family spelling bodies that can be
  delegated to f128 helpers, and focused output proof does not change.

### Step 5: Retire The F128 Markdown Shard

Goal: delete the markdown shard after its valid durable content has a compiled
home.

Primary targets:

- `src/backend/mir/aarch64/codegen/f128.md`
- `src/backend/mir/aarch64/codegen/f128.cpp`
- `src/backend/mir/aarch64/codegen/f128.hpp`
- `src/backend/mir/aarch64/codegen/README.md`, if it indexes shard ownership

Concrete actions:

- Confirm every valid implementation note from `f128.md` is represented by
  compiled f128 owner code, explicit fail-closed behavior, or another durable
  contract.
- Delete `f128.md`.
- Update any codegen shard inventory or ownership docs that would otherwise
  still list `f128.md` as active.

Completion check:

- `f128.md` is gone, `f128.cpp` / `f128.hpp` are the discoverable f128 owners,
  and no valid f128 behavior was left only in markdown.

### Step 6: Validate And Prepare Close Review

Goal: prove the redistribution is complete and did not become semantic drift.

Concrete actions:

- Run the supervisor-delegated build and focused backend/AArch64 proof.
- Escalate to broader backend or full CTest validation if multiple extraction
  packets have landed or the blast radius extends beyond the f128 route.
- Verify `instruction.cpp`, `dispatch.cpp`, and `machine_printer.cpp` satisfy
  the source idea completion criteria.
- Verify no reviewer reject signal from the source idea applies.

Completion check:

- Fresh proof is green or host-tool skips are explicitly reported, and the
  source idea is ready for close judgment.
