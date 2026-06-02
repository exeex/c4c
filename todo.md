Status: Active
Source Idea Path: ideas/open/88_aarch64_memory_frame_slot_address_materialization_owner.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map the Existing Frame-Slot Address Boundary

# Current Packet

## Just Finished

Activation created the active runbook for Step 1; no implementation packet has
run yet.

## Suggested Next

Delegate Step 1 to an executor. The packet should inspect the frame-slot address
helper cluster in `memory.cpp`/`memory.hpp`, choose the minimal owner
file/header shape, identify required call sites and f128/variadic touchpoints,
and write the narrow proof command into this file.

## Watchouts

- Keep the route AArch64-memory-local; do not publish generic BIR, prealloc,
  storage-plan, register-spelling, scratch-selection, or opcode authority.
- Do not fold `va_list` field addressing, cursor updates, dispatch
  publication, edge-copy, or prepared-wrapper work into this idea.
- Do not claim progress through helper movement, renames, or expectation
  rewrites without proving equivalent generated records and diagnostics.

## Proof

Not run; activation is lifecycle-only.
