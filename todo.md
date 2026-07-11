Status: Active
Source Idea Path: ideas/open/686_private_detail_header_contraction.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Private Detail Header Responsibilities

# Current Packet

## Just Finished

Lifecycle activation created this execution state for `plan.md` Step 1.

## Suggested Next

Read the source idea and handoff docs, then inventory
`src/backend/bir/lir_to_bir/lowering.hpp` declaration families before selecting
the first executor packet.

## Watchouts

Keep this slice behavior-preserving and adapter-private. Do not move LIR import
scratch state into public BIR, prepared/prealloc, target, or MIR ownership, and
do not absorb structured layout, initializer, memory/provenance, or call ABI
semantic work into this plan.

## Proof

No code proof run during lifecycle activation.
