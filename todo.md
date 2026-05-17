Status: Active
Source Idea Path: ideas/open/266_aarch64_atomics_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit The Atomic Shard And Current Route

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from the selected source idea.

## Suggested Next

Start Step 1 by auditing `src/backend/mir/aarch64/codegen/atomics.md` against
the current AArch64 codegen route, then record current facts, explicit
unsupported/deferred facts, obsolete reference-only material, and any concrete
owner or routing issue here before code changes.

## Watchouts

- Do not invent missing prepared atomic facts.
- Do not port the legacy exclusive-loop reference surface as a broad jump.
- Keep unsupported atomic forms explicit or fail-closed.
- Do not redistribute neighboring AArch64 markdown shards.
- Do not weaken tests or expectations to claim atomic support.

## Proof

Not run; lifecycle-only activation.
