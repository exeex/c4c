# Execution State

Status: Active
Source Idea Path: ideas/open/78_extract_x86_codegen_subsystem_to_markdown_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract Stable Entry Points And Shared Contracts
Plan Review Counter: 0 / 5
# Current Packet

## Just Finished

Lifecycle switched the active runbook from idea 75 to Phoenix stage 1 for the
whole `src/backend/mir/x86/codegen/` subsystem, and the first extraction
artifact now exists at
[x86_codegen_subsystem.md](/workspaces/c4c/docs/backend/x86_codegen_subsystem.md:1).
The artifact records the current subsystem boundary, the canonical lowering
families, the prepared-route family, the mixed contract pressure in
`x86_codegen.hpp`, and the initial classification of dispatch, core lowering,
compatibility, and overfit risk.

## Suggested Next

Deepen step 2 and step 3 by extracting a tighter representative contract set
from `x86_codegen.hpp` plus the canonical `calls.cpp` / `returns.cpp` /
`memory.cpp` seams, then add a more concrete prepared-divergence map for the
shape-specific ownership that currently lives in `prepared_module_emit.cpp`,
`prepared_param_zero_render.cpp`, `prepared_countdown_render.cpp`, and
`prepared_local_slot_render.cpp`.

## Watchouts

- Do not treat the extraction artifact as a place to redesign the subsystem
  yet.
- Do not dump whole `.cpp` or `.hpp` files into markdown; keep only important
  APIs, contracts, and short representative `cpp` blocks.
- Do not narrow the scope back down to `prepared_local_slot_render.cpp`; this
  route owns the whole `src/backend/mir/x86/codegen/` subsystem.
- Do not let the prepared family masquerade as “just a few helpers”; the
  artifact should keep calling out when it behaves like a parallel lowering
  stack.

## Proof

Extraction-stage documentation update on 2026-04-22. No build or test proof
required yet.
