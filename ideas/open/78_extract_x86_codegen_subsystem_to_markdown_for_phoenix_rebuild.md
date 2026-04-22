# Extract X86 Codegen Subsystem To Markdown For Phoenix Rebuild

Status: Open
Created: 2026-04-22
Last-Updated: 2026-04-22
Parent Idea: [75_post_link_prepared_call_lane_clobber_runtime_correctness_for_x86_backend.md](/workspaces/c4c/ideas/open/75_post_link_prepared_call_lane_clobber_runtime_correctness_for_x86_backend.md)

## Intent

Start the Phoenix rebuild for `src/backend/mir/x86/codegen/` by extracting the
current subsystem into a complete markdown artifact set instead of continuing
to patch the live implementation as if it were the design.

## Stage In Sequence

Stage 1 of 4: extraction.

## Produces

- `docs/backend/x86_codegen_legacy/index.md`
- `docs/backend/x86_codegen_legacy/alu.cpp.md`
- `docs/backend/x86_codegen_legacy/asm_emitter.cpp.md`
- `docs/backend/x86_codegen_legacy/atomics.cpp.md`
- `docs/backend/x86_codegen_legacy/calls.cpp.md`
- `docs/backend/x86_codegen_legacy/cast_ops.cpp.md`
- `docs/backend/x86_codegen_legacy/comparison.cpp.md`
- `docs/backend/x86_codegen_legacy/emit.cpp.md`
- `docs/backend/x86_codegen_legacy/f128.cpp.md`
- `docs/backend/x86_codegen_legacy/float_ops.cpp.md`
- `docs/backend/x86_codegen_legacy/globals.cpp.md`
- `docs/backend/x86_codegen_legacy/i128_ops.cpp.md`
- `docs/backend/x86_codegen_legacy/inline_asm.cpp.md`
- `docs/backend/x86_codegen_legacy/intrinsics.cpp.md`
- `docs/backend/x86_codegen_legacy/memory.cpp.md`
- `docs/backend/x86_codegen_legacy/mod.cpp.md`
- `docs/backend/x86_codegen_legacy/prepared_countdown_render.cpp.md`
- `docs/backend/x86_codegen_legacy/prepared_local_slot_render.cpp.md`
- `docs/backend/x86_codegen_legacy/prepared_module_emit.cpp.md`
- `docs/backend/x86_codegen_legacy/prepared_param_zero_render.cpp.md`
- `docs/backend/x86_codegen_legacy/prologue.cpp.md`
- `docs/backend/x86_codegen_legacy/returns.cpp.md`
- `docs/backend/x86_codegen_legacy/route_debug.cpp.md`
- `docs/backend/x86_codegen_legacy/shared_call_support.cpp.md`
- `docs/backend/x86_codegen_legacy/variadic.cpp.md`
- `docs/backend/x86_codegen_legacy/x86_codegen.hpp.md`

The extraction artifacts must:

- keep a one-to-one mapping from every in-scope legacy `.cpp` / `.hpp` file to
  its companion `.md`
- keep only important APIs, contracts, dependency directions, and short fenced
  `cpp` blocks for representative surfaces
- classify every notable special case as `core lowering`, `optional fast
  path`, `legacy compatibility`, or `overfit to reject`
- use the directory-level index to summarize the whole subsystem and point at
  every per-file artifact

## Does Not Yet Own

This stage does not own:

- judging the replacement architecture
- defining the planned replacement `.cpp.md` / `.hpp.md` layout
- drafting replacement files
- converting any markdown draft into real implementation
- deleting or migrating legacy x86 codegen sources

## Unlocks

This stage unlocks stage 2 review of the extracted subsystem model and gives
the rebuild a durable artifact set to critique instead of re-reading the live
`src/backend/mir/x86/codegen/` files ad hoc.

## Scope Notes

The extraction must cover the whole current x86 codegen subsystem, with
explicit attention to:

- the real entry points and pseudo-public contract surface exposed by
  `x86_codegen.hpp`, `mod.cpp`, and adjacent dispatcher files
- how responsibility is currently split across the arithmetic, memory, call,
  return, variadic, intrinsic, emitter, and prepared-route families
- where the `prepared_*.cpp` family duplicates, bypasses, or partially
  reimplements canonical x86 codegen seams
- which helpers are truly shared contracts versus accidental local growth
- what proof surfaces already exist in the repo for later migration stages

## Boundaries

This idea does not authorize redesign or implementation edits. It exists to
create the compressed legacy artifact set needed for honest subsystem review.

## Completion Signal

This idea is complete when every in-scope legacy
`src/backend/mir/x86/codegen/*.cpp` / `x86_codegen.hpp` file listed above has
its corresponding companion markdown artifact under
`docs/backend/x86_codegen_legacy/`, the directory-level
`docs/backend/x86_codegen_legacy/index.md` summarizes the subsystem and points
at the full set, and the extraction stays compressed instead of mechanically
dumping source into markdown.
