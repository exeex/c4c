Status: Active
Source Idea Path: ideas/open/edge-copy-block-entry-bookkeeping-prealloc.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Edge-Copy And Block-Entry Bookkeeping

# Current Packet

## Just Finished

Step 1 inventoried AArch64 edge-copy and block-entry bookkeeping and chose one
narrow first extraction boundary.

Inventory:

- `dispatch_edge_copies.cpp` mixes concrete AArch64 emission with some
  Prepared bookkeeping. Producer-chain walking, select-chain materialization,
  scratch-register selection, register spelling, frame-slot addressing, inline
  asm records, and scalar-state updates are AArch64-local. The small
  `prepared_edge_select_source_is_destination_register(...)` comparison reads
  Prepared homes but depends on target register spelling, so it is not the
  first move.
- `dispatch_publication.cpp` has repeated target-neutral scans over
  `PreparedMoveBundle` records for `BlockEntry` plus
  `OutOfSsaParallelCopy` authority and the current block label. Those scans
  distinguish value destinations, register storage, `to_value_id`, source
  bundle labels, missing destination register fields, and value-home fallback.
  The follow-on register parsing, alias checks, scalar-state recording,
  load/store/inline-asm text, and diagnostics remain AArch64-local.
- `dispatch_producers.cpp` repeats the same block-entry out-of-SSA
  publication scan to decide whether a named producer should use an incoming
  block-entry publication. The recursive same-block producer analysis is
  target-local lowering strategy; the Prepared bundle filtering is shared
  bookkeeping.
- `prealloc/value_locations.hpp` already owns `PreparedMoveBundle`,
  `PreparedMoveResolution`, value homes, move phases, authority kind, and
  generic find helpers by phase/block/instruction.
- `prealloc/control_flow.hpp` already owns route-local out-of-SSA
  parallel-copy lookup by predecessor/successor labels and execution block.
- `prealloc/prepared_lookups.hpp/.cpp` already owns indexed value-home and
  move-bundle lookup surfaces, but not a decoded block-entry publication view.

Chosen boundary: add a prealloc helper that collects/classifies block-entry
out-of-SSA value publications for one successor block label from
`PreparedValueLocationFunction` and value homes. The helper should return
fact/status records preserving the source bundle, move, home, destination
value id/name, destination storage kind, and register-name availability without
parsing target registers or constructing MIR operands.

Destination layer: prealloc, because the boundary consumes Prepared facts
directly (`PreparedMoveBundle`, `PreparedMoveResolution`, `PreparedValueHome`,
block-label authority). It is not shared MIR yet; the data has not become a
target-independent machine-record concern.

Target-local responsibilities: AArch64 keeps register parsing/spelling,
physical-register aliasing, scalar-state recording, scratch choice, load/store
mnemonics, frame-pointer vs stack-pointer address policy, inline asm and
machine-instruction record construction, concrete edge-copy emission, and
diagnostic behavior.

## Suggested Next

Step 2 should add the selected prealloc block-entry publication helper without
adapting AArch64 yet. Focus direct helper coverage on available register
publications, stack/non-register destinations, missing home/register facts,
wrong phase/authority/label fallback, and preservation of source Prepared
records.

## Watchouts

Keep the first helper scoped to block-entry out-of-SSA publication facts. Do
not move edge producer walking, select-chain materialization, target register
spelling/parsing, scratch-register selection, machine records, target operands,
or move diagnostics into prealloc.

Avoid a shared-MIR helper for this first slice: the repeated code is still
Prepared fact consumption, not target-independent machine-record bookkeeping.

## Proof

Documentation/inventory-only packet. No implementation files were edited, so
no build or ctest proof was required or run.
