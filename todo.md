Status: Active
Source Idea Path: ideas/open/24_riscv_prepared_edge_publication_register_destination_consumer.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory RISC-V Edge Publication Consumer Path

# Current Packet

## Just Finished

Completed Step 1 inventory for the RISC-V register-destination edge
publication route.

Chosen first source-home family: `Register -> Register`. This is the smallest
RISC-V slice because it can render the source and destination through prepared
register names and emit a target-local `mv <dst>, <src>` without introducing
stack width policy, immediate materialization policy, or pointer address
materialization.

Shared prepared lookup access point: use the same authority shape as x86,
`prepare::find_unique_indexed_prepared_edge_publication(&lookups.edge_publications,
predecessor_label, successor_label, destination_value_id)`, after obtaining
per-function shared prepared lookups from the prepared module. The RISC-V route
must consume this lookup result; it must not rescan predecessor/successor blocks
or rediscover edge-copy facts locally.

Target files/functions for the next code packet:

- `src/backend/mir/riscv/codegen/emit.hpp`: declare a RISC-V prepared-module or
  prepared-edge publication entrypoint.
- `src/backend/mir/riscv/codegen/emit.cpp`: add the RISC-V consumer helper that
  reads shared `edge_publications`, validates register destination and register
  source homes, and renders `mv`.
- `src/backend/backend.cpp` and `src/backend/backend.hpp`: add/wire an
  explicit RISC-V prepared BIR/LIR entrypoint when the implementation is ready,
  analogous to the existing x86/AArch64 target-local handoff boundary.
- Focused future tests should live beside the existing backend route and
  prepared lookup tests, with assertions that fail if RISC-V ignores shared
  edge-publication lookups.

Current unsupported/fail-closed behavior: RISC-V does not have a target-local
prepared-module consumer for shared `edge_publications`. Non-x86/non-AArch64
BIR/LIR emission currently falls back to generic prepared-BIR text after
`prepare_semantic_bir_pipeline`, and `src/backend/mir/riscv/codegen/emit.cpp`
only exposes a direct prepared-LIR tiny-add bootstrap slice that rejects other
LIR shapes. There is no RISC-V helper that turns shared edge-publication facts
into edge moves yet.

Homes that stay explicitly unsupported for this scoped first slice:
`StackSlot -> Register`, `RematerializableImmediate -> Register`,
`PointerBasePlusOffset -> Register`, and any source-to-`StackSlot`
destination. Missing shared lookups, missing publication authority, non-move
publication operations, missing source homes, missing destination homes, and
non-register destinations should all fail closed rather than falling back to
local guessing.

## Suggested Next

Implement Step 2 for `Register -> Register` only: add the RISC-V shared lookup
consumer helper and emit `mv <destination_register>, <source_register>` for an
available block-entry parallel-copy edge publication with a register
destination.

## Watchouts

- Keep this plan scoped to register-destination edge-publication consumers.
- Preserve fail-closed behavior for pointer-base sources and stack-slot
  destinations unless a later lifecycle decision changes scope.
- Do not rediscover edge-copy facts in RISC-V; consume shared prepared
  `edge_publications`.
- Do not fold stack or immediate source support into the first code packet; it
  would mix source-home policy with the initial RISC-V handoff boundary.

## Proof

docs/inventory-only. No build or tests were required, and no `test_after.log`
was created.
