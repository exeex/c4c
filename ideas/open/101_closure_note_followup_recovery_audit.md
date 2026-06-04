# Closure Note Follow-Up Recovery Audit

## Goal

Audit recent closure notes for follow-up ideas that were generated in notes but
are missing, incomplete, or not yet represented as actionable `ideas/open/*`
files, then produce the next focused ideas needed to finish that unfinished
work.

## Why This Exists

Several recent closure notes say they generated follow-up ideas, but
`ideas/open/` is currently empty after the latest completed batch. In
particular:

- `ideas/closed/97_bir_prealloc_call_abi_authority_boundary_audit.md` lists
  follow-ups originally named `100` through `103`.
- `ideas/closed/98_bir_prealloc_memory_pointer_storage_boundary_audit.md` lists
  follow-ups originally named `104` through `107`.
- `ideas/closed/99_bir_prealloc_control_publication_lookup_boundary_audit.md`
  leaves `108_prepared_select_chain_dump_contract_coverage.md` open in the
  note, but no such open file exists now.

Before continuing x86/RISC-V rebuild preparation, this gap should be audited
deliberately. The task is not to implement all missing follow-ups in one pass;
it is to recover the durable intent and emit small, numbered ideas with clear
ownership and proof routes.

## Owned Files

- Audit/read:
  - `ideas/closed/97_bir_prealloc_call_abi_authority_boundary_audit.md`
  - `ideas/closed/98_bir_prealloc_memory_pointer_storage_boundary_audit.md`
  - `ideas/closed/99_bir_prealloc_control_publication_lookup_boundary_audit.md`
  - any closure notes after `99` if they exist by the time this idea runs
  - `src/backend/bir/**`
  - `src/backend/prealloc/**`
  - `src/backend/mir/aarch64/codegen/**` only for consumer context
- Write:
  - new `ideas/open/*.md` follow-ups only

## In Scope

- Inventory every closure-note follow-up that is missing from `ideas/open/`.
- Decide whether each missing follow-up is:
  - still needed as-is
  - already completed by later work
  - superseded by a better-scoped idea
  - stale and should be recorded as no-action
- Recreate still-needed work as new focused ideas using the next available
  numbering at execution time.
- Preserve the distinction between:
  - BIR semantic authority
  - prealloc prepared-plan/storage authority
  - arch backend consumer authority
- Produce a short close note mapping each closure-note item to its final
  disposition.

## Out Of Scope

- Implementing the recovered follow-up ideas.
- Editing compiler implementation files.
- Moving old closed ideas back to open without reviewing whether later work
  superseded them.
- Creating broad "clean up BIR/prealloc" ideas without exact fact ownership.

## Proof Expectations

- Analysis-only; no backend tests are required unless implementation files are
  accidentally changed.
- The proof is the generated set of focused `ideas/open/*.md` files plus a
  mapping table in the close note.

## Reviewer Reject Signals

- The audit blindly recreates all note-listed ideas without checking later
  closure work.
- It loses numbering discipline or overwrites existing ideas.
- It creates implementation tasks without naming the duplicated or missing
  authority fact.

