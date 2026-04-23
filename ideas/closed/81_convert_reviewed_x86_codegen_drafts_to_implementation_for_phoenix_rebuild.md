# Convert Reviewed X86 Codegen Drafts To Implementation For Phoenix Rebuild

Status: Open
Created: 2026-04-22
Last-Updated: 2026-04-23
Parent Idea: [80_draft_replacement_x86_codegen_interfaces_for_phoenix_rebuild.md](/workspaces/c4c/ideas/open/80_draft_replacement_x86_codegen_interfaces_for_phoenix_rebuild.md)

## Intent

Convert the reviewed x86 codegen replacement drafts into real
`src/backend/mir/x86/codegen/` implementation through staged migration.

## Stage In Sequence

Stage 4 of 4: draft-to-implementation conversion.

## Produces

- real `.cpp` / `.hpp` implementation under `src/backend/mir/x86/codegen/`
  that matches the reviewed draft set from
  `docs/backend/x86_codegen_rebuild/`
- dispatcher and ownership rewiring that routes prepared and non-prepared
  lowering through the reviewed seams
- deletion or retirement of legacy x86 codegen paths only after the
  replacement owners are live and proved

## Does Not Yet Own

This stage does not own:

- changing the reviewed draft contract without first repairing stage 3
- deleting legacy logic before the new owner is actually in use and proved
- redefining the replacement architecture outside the reviewed draft set

## Unlocks

This stage unlocks eventual retirement of the old subsystem shape once the new
ownership graph is live, proven, and no longer depends on the legacy routes.

## Scope Notes

Migration should typically move shared helpers first, then coherent lowering
families, then dispatcher rewiring, and only then legacy deletion. Every
packet should make it clear what responsibility moved and what remains in the
legacy route.

## Boundaries

Keep the old route available until the reviewed replacement seams actually own
the migrated behavior.

## Latest Durable Note

As of 2026-04-23, the active phoenix-rebuild runbook reached its final
step-5 structural checkpoint: the residual top-level compatibility surface was
explicitly classified, and the broader proof command
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure > test_after.log`
held the repo at the same total as `test_before.log`
(`3187 passed / 107 failed / 3294 total`), so the rebuild work did not
introduce a new regression. The remaining failures in that proof plateau are
still owned by separate backend capability ideas, led by repeated
authoritative prepared local-slot and guard-chain handoff families plus a
small remaining load-local-memory family. Park this stage-4 implementation
idea until those downstream leaves advance enough for a fresh broader
milestone proof to support closure.

## Completion Signal

This idea is complete when the reviewed replacement draft set has been
converted into real x86 codegen implementation, the new ownership seams are
actually in use, remaining legacy code is explicitly classified, and proof
shows the migrated capability families still work.
