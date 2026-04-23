# Full X86 Backend Contract-First Replan

Status: Open
Created: 2026-04-23
Last-Updated: 2026-04-23
Supersedes: [82_extract_full_x86_backend_subsystem_to_markdown_for_phoenix_rebuild.md](/workspaces/c4c/ideas/open/82_extract_full_x86_backend_subsystem_to_markdown_for_phoenix_rebuild.md)

## Intent

Replan the whole `src/backend/mir/x86/` backend around contract-first
ownership, explicit subsystem boundaries, and in-place markdown design
artifacts before the next behavior-recovery packets land.

## Scope

- `src/backend/README.md`
- `src/backend/mir/x86/README.md`
- `src/backend/mir/x86/`
- contract READMEs for `assembler/` and `linker/`
- key in-place `*.cpp.md` / `*.hpp.md` that define x86 public or subsystem
  interfaces

## Non-Goals

- fully restoring legacy x86 behavior in one packet
- hiding current BIR capability gaps behind x86-specific workaround policy
- treating extraction-only progress as sufficient once live ownership changed

## Success Signal

This idea is complete when the active x86 backend tree has:

- one honest root architecture contract
- subsystem-level READMEs for live and deferred x86 owners
- live x86 public surfaces that compile through thin, explicit seams
- key markdown companions promoted from raw extraction evidence to design
  contract plus legacy evidence
- follow-on behavior recovery clearly separated from contract/layout work

## Latest Durable Note

As of 2026-04-23, the contract-first x86 layout pass established the new root
ownership graph, shrank filenames, enforced one non-helper header per x86
subdirectory, and kept `c4c_backend` compiling through the thinner public
surface. The next meaningful gaps discovered during execution were upstream of
further x86 behavior recovery: `out_of_ssa` phase authority, prepared
frame/stack/call authority completion, and grouped-register prealloc authority.
Those follow-on seams now live under ideas 87, 88, and 89 instead of being
silently absorbed back into the x86 layout runbook.
