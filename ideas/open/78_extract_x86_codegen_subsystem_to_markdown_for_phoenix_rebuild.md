# Extract X86 Codegen Subsystem To Markdown For Phoenix Rebuild

Status: Open
Created: 2026-04-22
Last-Updated: 2026-04-22
Parent Idea: [75_post_link_prepared_call_lane_clobber_runtime_correctness_for_x86_backend.md](/workspaces/c4c/ideas/open/75_post_link_prepared_call_lane_clobber_runtime_correctness_for_x86_backend.md)

## Intent

Start a Phoenix rebuild for `src/backend/mir/x86/codegen/` by extracting the
current subsystem into a compressed markdown design artifact instead of
continuing to grow local fixes inside the live implementation.

## Stage In Sequence

Stage 1 of 4: extraction.

## Produces

- `docs/backend/x86_codegen_subsystem.md`

The artifact must:

- convert the current `src/backend/mir/x86/codegen/*.cpp` family and
  `src/backend/mir/x86/codegen/x86_codegen.hpp` into markdown
- keep only important APIs, contracts, dependency directions, and
  representative short fenced `cpp` blocks
- classify special cases as `core lowering`, `optional fast path`,
  `legacy compatibility`, or `overfit to reject`

## Does Not Yet Own

This stage does not own:

- redesigning the subsystem
- drafting new `.cpp.md` / `.hpp.md` replacement files
- converting any draft into real implementation
- deleting or migrating legacy x86 codegen sources

## Unlocks

This stage unlocks stage 2 review of the extracted subsystem model and creates
the durable artifact the later redesign must critique instead of treating the
live `.cpp` files as the design.

## Scope Notes

The extraction must cover the whole current x86 codegen subsystem, with
special attention to:

- how `x86_codegen.hpp` acts as the public or pseudo-public contract surface
- how responsibilities are currently split across `calls.cpp`, `returns.cpp`,
  `memory.cpp`, `emit.cpp`, `mod.cpp`, and the `prepared*.cpp` family
- where the `prepared*.cpp` route duplicates or bypasses already-established
  seams from the rest of x86 codegen
- which helpers are truly shared contracts versus accidental local growth

## Boundaries

This idea does not authorize implementation edits. It exists to create the
compressed subsystem model needed for honest review and replacement planning.

## Completion Signal

This idea is complete when `docs/backend/x86_codegen_subsystem.md` exists and
captures the important entry points, ownership buckets, hidden dependencies,
representative contracts, and special-case classifications for the current
x86 codegen subsystem without mechanically dumping source files into markdown.
