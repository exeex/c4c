Status: Active
Source Idea Path: ideas/open/261_aarch64_f128_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit F128 Ownership And Build Surface

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from the source idea. No
implementation packet has run yet.

## Suggested Next

Start Step 1 by auditing the f128 markdown shard, broad AArch64 codegen owners,
and the build-source list. Classify ownership before moving code.

## Watchouts

- Keep the redistribution behavior-preserving.
- Preserve fail-closed and deferred-helper boundaries.
- Do not weaken tests, rewrite expectations, or add named-case f128 shortcuts.
- Do not redesign `instruction.hpp` solely to split this shard.

## Proof

Not run; lifecycle activation only.
