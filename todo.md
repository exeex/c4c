# Execution State

Status: Active
Source Idea Path: ideas/open/78_extract_x86_codegen_subsystem_to_markdown_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract Stable Entry Points And Shared Contracts
Plan Review Counter: 1 / 5
# Current Packet

## Just Finished

Advanced plan step 2, `Extract Stable Entry Points And Shared Contracts`, by
tightening
[x86_codegen_subsystem.md](/workspaces/c4c/docs/backend/x86_codegen_subsystem.md:1)
around the real shared contract clusters: the `PreparedX86FunctionDispatchContext`
surface in `x86_codegen.hpp`, the header-level prepared entrypoints and route
tracing declarations, and the concrete ownership already present in canonical
`calls.cpp`, `returns.cpp`, and `memory.cpp`. The artifact now also records a
clearer prepared-divergence map for `prepared_module_emit.cpp`,
`prepared_param_zero_render.cpp`, `prepared_countdown_render.cpp`, and
`prepared_local_slot_render.cpp`, including which prepared metadata families
each file consumes directly.

## Suggested Next

Finish the remaining step 2 / step 3 extraction by compressing one or two more
representative contract blocks from the prepared family itself, especially the
call-lane and same-module data-emission helpers inside
`prepared_module_emit.cpp` and `prepared_local_slot_render.cpp`, then prune any
markdown that still reads like file-by-file narration instead of subsystem
ownership.

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
required yet; this slice only updated
[x86_codegen_subsystem.md](/workspaces/c4c/docs/backend/x86_codegen_subsystem.md:1)
and `todo.md`.
