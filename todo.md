Status: Active
Source Idea Path: ideas/open/520_bir_render_owner_preservation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Render Helper Consumers

# Current Packet

## Just Finished

Lifecycle activation created the runbook for idea 520 from the BIR render owner
preservation source idea. No implementation work has started.

## Suggested Next

Delegate or execute Step 1 - Audit Render Helper Consumers. Start by loading
`.codex/skills/c4c-clang-tools/SKILL.md`, confirming the clang tools are on
`PATH`, and mapping callers/callees for `render_type`,
`render_binary_opcode`, and `render_cast_opcode`.

## Watchouts

This is a behavior-preserving cleanup. Do not move route declarations, route
records, route validation, printer output, validator behavior, public
signatures, tests, expectations, or gcc_torture accounting. If the render helper
consumer map does not justify a move, preserve the bodies in `bir.cpp` and
record why.

## Proof

Lifecycle-only activation; no build or test proof required yet.
