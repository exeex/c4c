Status: Active
Source Idea Path: ideas/open/258_aarch64_emit_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Emit Shard Content

# Current Packet

## Just Finished

Lifecycle activation created this canonical execution state from `plan.md`.

## Suggested Next

Start Step 1 by classifying `src/backend/mir/aarch64/codegen/emit.md` content
against the current compiled owners before changing implementation files.

## Watchouts

- Preserve behavior; this idea is source ownership redistribution.
- Do not use testcase-shaped shortcuts, expectation rewrites, or unsupported
  downgrades as progress.
- Keep `emit.cpp` thin and avoid turning it into a catch-all for instruction
  spelling, selected-node families, or backend semantics.
- Do not delete `emit.md` until its durable valid content is reconciled or
  classified as stale/out of scope for close.

## Proof

Not run; lifecycle activation only.
