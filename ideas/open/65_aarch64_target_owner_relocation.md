# AArch64 Target Owner Relocation

Ownership class: target-owner split

## Goal

Relocate AArch64 target emission helpers for globals, publication spelling, and
value materialization out of broad dispatch-family files into precise local
owners without moving semantic authority to shared code.

## Why This Exists

Idea 59 classified several target-local groups as justified AArch64 behavior
that still lives under overly broad dispatch filenames. These helpers own
instruction spelling, register and scratch hazards, frame-address rendering,
global symbol spelling, and retry routing. The cleanup is a target-owner split,
not a shared semantic migration.

## In Scope

- `src/backend/mir/aarch64/codegen/dispatch_publication.*` target-spelling and
  prepared-consumer groups.
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.*` prepared
  global, load-local, local-slot, scratch, and write-hazard consumers.
- `src/backend/mir/aarch64/codegen/dispatch_producers.*` load-global target and
  symbol helpers.
- `src/backend/mir/aarch64/codegen/dispatch.cpp` address-materialization retry
  routing.
- Precise local owners such as `globals.cpp`, `memory.cpp`, `alu.cpp`,
  publication/address helpers, or a non-dispatch value-materialization owner.

## Out Of Scope

- Moving target-local clobber, register spelling, instruction spelling, or
  frame-address rendering to shared code.
- Changing semantic producer discovery or prepared/shared query authority.
- Bulk merging target helpers into `dispatch.cpp`.
- Contracting the local block dispatch route before producer, publication, and
  value-materialization authority is clear.

## Acceptance Criteria

- Target emission helpers move to precise AArch64 owners that match their
  domain, such as globals, memory, ALU, publication, address, or value
  materialization.
- Public dispatch-family surfaces shrink only where call sites can use the new
  precise owner directly.
- Scratch-register hazards, local-slot and global spelling, fixed-formal
  stores, GOT/direct labels, scalar load/store mnemonics, and retry routing
  retain behavior.
- Focused proof covers global load spelling, GOT/direct symbol labels,
  frame-slot address spelling, fixed-formal stores, scalar load/store
  mnemonics, local-slot/global value materialization, scratch-register hazards,
  and address-materialization retry paths. Escalate if public helper headers are
  deleted or target emission is redistributed across multiple compilation
  units.

## Reviewer Reject Signals

- The route turns into a bulk merge into `dispatch.cpp`.
- Target-local clobber or register-spelling behavior moves to shared code.
- Helper relocation changes semantic producer discovery.
- Implementation claims contraction while leaving public dispatch-family
  surfaces unchanged.
- The same broad dispatch ownership remains hidden behind newly named files or
  forwarding helpers.
