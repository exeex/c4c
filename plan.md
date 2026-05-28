# AArch64 Target Owner Relocation Runbook

Status: Active
Source Idea: ideas/open/65_aarch64_target_owner_relocation.md

## Purpose

Relocate AArch64 target emission helpers out of broad dispatch-family files and
into precise target-local owners without moving semantic authority to shared
code.

## Goal

AArch64 globals, publication spelling, value materialization, local-slot,
scratch, and retry helpers should live under owners that match their emission
domain while preserving existing target behavior.

## Core Rule

This is a target-owner split, not a shared-authority migration. Do not move
AArch64 clobber handling, register spelling, instruction spelling,
frame-address rendering, or target retry behavior to shared code, and do not
change semantic producer discovery.

## Read First

- `ideas/open/65_aarch64_target_owner_relocation.md`
- `src/backend/mir/aarch64/codegen/dispatch_publication.*`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.*`
- `src/backend/mir/aarch64/codegen/dispatch_producers.*`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- Existing precise owners such as `globals.*`, `memory.*`, `alu.*`,
  `prepared_value_home_materialization.*`, and address/publication helpers.

## Current Targets

- `dispatch_publication.*` target-spelling and prepared-consumer groups.
- `dispatch_value_materialization.*` prepared global, load-local, local-slot,
  scratch, and write-hazard consumers.
- `dispatch_producers.*` load-global target and symbol helpers.
- `dispatch.cpp` address-materialization retry routing.
- Precise local owners for globals, memory, ALU, publication, address, and
  value materialization behavior.

## Non-Goals

- Do not move target-local clobber, register spelling, instruction spelling,
  frame-address rendering, or retry routing to shared code.
- Do not change semantic producer discovery or prepared/shared query authority.
- Do not bulk merge target helpers into `dispatch.cpp`.
- Do not contract the local block dispatch route before producer,
  publication, and value-materialization ownership is clear.
- Do not claim progress through forwarding-only renames that leave broad
  dispatch ownership intact.

## Working Model

The source idea follows earlier dispatch-family classification work. Some
helpers still live under broad dispatch filenames even though they are
legitimate AArch64 target emission behavior. The route should split these
helpers into precise AArch64 owners while preserving behavior, public call
contracts where still needed, and target-local hazard decisions.

Each move should be small enough to prove with focused backend tests and route
quality scans. If a helper turns out to own semantic prepared/query authority
rather than target emission, stop and report the ownership mismatch instead of
moving it as part of this idea.

## Execution Rules

- Start with an inventory before moving code.
- Move helpers only to owners that match their actual target-emission domain.
- Prefer direct public-surface shrinkage where call sites can use the precise
  owner directly.
- Keep behavior-preserving wrappers only as temporary adapters when deleting
  them would widen the packet.
- Preserve scratch-register hazards, local-slot and global spelling,
  fixed-formal stores, GOT/direct labels, scalar load/store mnemonics, and
  address-materialization retry behavior.
- Run `git diff --check` for code-changing packets.
- Run at least the supervisor-selected backend proof for each code-changing
  packet; escalate if public helper headers are deleted or target emission is
  redistributed across multiple compilation units.

## Ordered Steps

### Step 1: Inventory Target-Owner Relocation Candidates

Goal: classify the broad dispatch-family helpers by precise AArch64 target
owner, call sites, and proof needs before code moves.

Primary targets:
`dispatch_publication.*`, `dispatch_value_materialization.*`,
`dispatch_producers.*`, and `dispatch.cpp`.

Actions:
- List helpers that own globals, publication spelling, value materialization,
  local-slot addressing, scratch/write hazards, load/store mnemonics, and
  address-materialization retry routing.
- Identify current public declarations and call sites for each helper group.
- Classify each helper as target-local emission, temporary adapter, or
  out-of-scope semantic authority.
- Choose precise destination owners for the first movable group.
- Record proof needs for global load spelling, GOT/direct labels, frame-slot
  address spelling, fixed-formal stores, scalar load/store mnemonics,
  local-slot/global materialization, scratch hazards, and retry routing.
- Update `todo.md` with the inventory and the first coherent relocation packet.

Completion check:
- `todo.md` records the helper inventory, selected first target-owner packet,
  public-surface notes, and proof requirements without implementation edits.

### Step 2: Relocate Global And Symbol Target Helpers

Goal: move load-global target and symbol-spelling helpers to precise AArch64
global/publication owners while preserving GOT/direct label behavior.

Primary targets:
`dispatch_producers.*`, `dispatch_value_materialization.*`, and `globals.*`.

Actions:
- Move helpers that only spell global symbols, GOT/direct labels, or
  load-global target emission to a precise global owner.
- Update call sites to include/use the precise owner directly where possible.
- Shrink dispatch-family declarations that become unnecessary.
- Preserve existing global load materialization and symbol spelling behavior.
- Avoid changing semantic producer discovery or prepared query ownership.

Completion check:
- Global and symbol target helpers no longer require broad dispatch ownership,
  and focused backend proof covers GOT/direct symbol labels and global load
  materialization.

### Step 3: Relocate Local-Slot, Frame-Address, And Memory Spelling Helpers

Goal: move local-slot, frame-address, scalar load/store mnemonic, and
fixed-formal store emission helpers to precise memory/address owners.

Primary targets:
`dispatch_publication.*`, `dispatch_value_materialization.*`, `memory.*`, and
address/publication helper files.

Actions:
- Move helpers that render frame-slot addresses, local-slot/global value
  materialization, fixed-formal stores, scalar load/store mnemonics, or related
  memory spelling.
- Keep stack-layout and fixed-frame-pointer behavior target-local.
- Update public declarations and call sites to use the precise owner.
- Preserve local-slot and aggregate address behavior; do not bundle unrelated
  null-path cleanup from idea 67.

Completion check:
- Memory/address spelling helpers live under precise AArch64 owners, and proof
  covers frame-slot address spelling, fixed-formal stores, scalar load/store
  mnemonics, and local-slot value materialization.

### Step 4: Relocate Scratch, Write-Hazard, And Publication Spelling Helpers

Goal: move scratch-register, write-hazard, and publication-spelling helpers to
precise target-local owners without changing clobber behavior.

Primary targets:
`dispatch_publication.*`, `dispatch_value_materialization.*`, ALU/publication
owners, and prepared value-home materialization helpers.

Actions:
- Move helpers whose authority is target-local scratch choice, write-hazard
  handling, prepared publication spelling, or register-form emission.
- Keep clobber, register spelling, and instruction spelling local to AArch64.
- Remove obsolete dispatch-family declarations once call sites use the precise
  owner.
- Preserve value materialization retry behavior and scalar register semantics.

Completion check:
- Scratch and publication spelling helpers are owned by precise AArch64 files,
  and focused proof covers scratch-register hazards, publication spelling, and
  representative prepared value materialization.

### Step 5: Retire Dispatch-Family Adapter Surface

Goal: shrink broad dispatch-family public surfaces after helper relocation,
leaving only justified route-level entry points.

Primary targets:
public declarations in `dispatch_publication.hpp`,
`dispatch_value_materialization.hpp`, `dispatch_producers.hpp`, and remaining
call sites.

Actions:
- Delete temporary forwarding helpers whose call sites can use precise owners.
- Keep wrappers only when they represent a deliberate route-level adapter.
- Scan for broad dispatch ownership hidden behind newly named files or
  forwarding helpers.
- Confirm `dispatch.cpp` did not accumulate relocated helper logic.

Completion check:
- Public dispatch-family surfaces are smaller or explicitly justified, and no
  renamed helper preserves the old broad ownership pattern.

### Step 6: Prove Target-Owner Preservation

Goal: validate the relocation set without overfitting one target-helper case.

Primary targets:
focused AArch64/backend tests and route-quality scans.

Actions:
- Run the supervisor-delegated proof command after the final relocation packet.
- Run `git diff --check`.
- Scan for broad dispatch ownership, forwarding-only relocations, or target
  helper logic moved into `dispatch.cpp`.
- Confirm proof covers global load spelling, GOT/direct labels, frame-slot
  address spelling, fixed-formal stores, scalar load/store mnemonics,
  local-slot/global value materialization, scratch-register hazards, and
  address-materialization retry paths.
- Escalate validation if public helper headers are deleted or target emission
  moved across multiple compilation units.

Completion check:
- Backend proof and route-quality scans are green, and the diff contains no
  shared-authority drift, semantic producer-discovery changes, or broad
  dispatch-owner rename.
