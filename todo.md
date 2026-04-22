# Execution State

Status: Active
Source Idea Path: ideas/open/78_extract_x86_codegen_subsystem_to_markdown_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract Canonical Entry Points, Contracts, And Lowering Families
Plan Review Counter: 0 / 5
# Current Packet

## Just Finished

Repaired the Phoenix extraction contract: stage 1 no longer closes on one
summary markdown file. It now closes only when every in-scope
`src/backend/mir/x86/codegen/*.cpp` / `x86_codegen.hpp` has its own companion
`.md` and one directory-level index `.md` ties the set together. The
lifecycle artifacts and the `phoenix-rebuild` skill now agree on that shape,
and stage 3 likewise requires one `.cpp.md` / `.hpp.md` per planned
replacement file.

## Suggested Next

Replace the single-file extraction output with a real artifact set under
`docs/backend/`: one `.md` for each in-scope x86 codegen `.cpp` / `.hpp`, plus
one directory-level index `.md` that summarizes ownership, dependency
direction, and prepared-route divergence across the set.

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
- Do not accept stage-1 completion until coverage is complete at the
  file-to-markdown level.

## Proof

Lifecycle and skill-contract update on 2026-04-22. No build or test proof
required yet; this slice only updated planning/skill documents.
