Status: Active
Source Idea Path: ideas/open/169_route3_semantic_memory_access_cache_split.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Candidate Route 3 Consumers

# Current Packet

## Just Finished

Lifecycle activation initialized this packet from Step 1 of `plan.md`.

## Suggested Next

Execute Step 1: inventory prepared memory/access consumers, classify
candidates, and select one bounded semantic consumer family for the first
Route 3 migration packet.

## Watchouts

- Keep the first packet analysis-only unless the supervisor delegates
  implementation.
- Do not copy `PreparedAddress`, `PreparedMemoryAccess`, frame-slot ids,
  offsets, relocation/TLS details, base-plus-offset legality, or memory operand
  formation into BIR.
- Do not hide prepared memory/addressing helpers while unaudited consumers
  still depend on them.
- Reject broad rescans, target-addressing migrations, or named-case-only
  shortcuts.

## Proof

Not run; lifecycle activation only.
