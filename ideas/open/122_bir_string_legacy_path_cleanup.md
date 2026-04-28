# BIR String Legacy Path Cleanup

Status: Open
Created: 2026-04-28

Prerequisite Ideas:
- [121_x86_prepared_module_renderer_recovery.md](/workspaces/c4c/ideas/open/121_x86_prepared_module_renderer_recovery.md)

## Goal

Remove or demote legacy string authority from BIR after x86 prepared-module
renderer recovery has been closed, so BIR semantic identity is carried by
structured ids and typed records rather than raw spelling.

## Why This Idea Exists

The compiler pipeline is layered as:

```text
C++ -> HIR -> LIR -> BIR -> MIR -> ASM
```

MIR and target assembly generation are expected to be rewritten, so the next
mainline cleanup should not depend on current MIR/ASM behavior as the primary
acceptance surface. The durable contract to clean first is BIR, then the
LIR-to-BIR boundary, and only later higher frontend layers.

Idea 121 repairs the x86 renderer and x86 backend BIR/handoff tests so they can
serve as a useful validation surface. It is not a license to expand this idea
into renderer recovery. If this cleanup exposes an x86/MIR acceptance hole that
belongs to unfinished renderer work, open a follow-up idea, expected as idea
123, and keep this cleanup focused on BIR string authority.

## Sequencing Rule

- Complete and close idea 121 before activating this idea.
- Start this idea as the return to the mainline string-legacy cleanup.
- If execution finds a required x86/MIR renderer or handoff gap that was not
  fully resolved by idea 121, do not absorb that work here. Record the blocker
  and create a separate follow-up idea, expected as idea 123, for the backfill.

## Scope

- Inventory string-valued BIR fields, helpers, builders, printers, verifiers,
  tests, and LIR-to-BIR lowering boundaries.
- Classify each string use as display spelling, selector/user input,
  compatibility payload, unresolved-id boundary, or semantic authority.
- Remove or demote BIR string uses that still act as semantic authority for
  labels, symbols, types, layout, function identity, or value identity.
- Move ordinary BIR semantic identity to structured ids, typed records, or
  explicit resolver tables where the data already exists upstream.
- Keep BIR dump spelling stable unless a separate contract change is approved.
- Use BIR, BIR verifier, BIR printer, focused dump, and LIR-to-BIR tests as the
  primary acceptance surface.

## Non-Goals

- Do not rewrite MIR or target x86 assembly generation in this idea.
- Do not use current MIR/ASM output as the primary proof that BIR string
  authority was removed.
- Do not start parser or HIR cleanup before the BIR and LIR-to-BIR authority
  boundary is clear.
- Do not delete strings that are retained only for display spelling, selector
  input, stable dumps, compatibility fixtures, or intentionally unresolved ids.
- Do not mark supported tests unsupported or weaken expectations to make string
  cleanup pass.

## Acceptance Criteria

- BIR has a documented map of retained string fields and each retained use is
  classified as display, selector, compatibility, unresolved, or another
  explicit non-authority boundary.
- Ordinary BIR semantic identity no longer depends on raw string comparison
  where structured ids or typed records are available.
- LIR-to-BIR lowering carries structured identity into BIR instead of requiring
  BIR to rediscover ordinary semantics from spelling.
- Focused BIR, BIR printer, BIR verifier, and LIR-to-BIR tests cover both the
  structured path and any intentionally retained string compatibility boundary.
- Any blocker that belongs to unfinished x86/MIR renderer recovery is recorded
  as a separate follow-up idea rather than absorbed into this cleanup.
