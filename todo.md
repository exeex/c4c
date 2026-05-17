Status: Active
Source Idea Path: ideas/open/259_aarch64_cast_ops_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Move cast construction and lowering behavior

# Current Packet

## Just Finished

Activated the paused cast-ops redistribution runbook after closure of idea 263.
The linked source idea says Step 2 already introduced the compiled
`cast_ops.cpp`/`cast_ops.hpp` owner shell and build registration; execution
should resume at Step 3.

## Suggested Next

Delegate Step 3 to an executor: move cast-specific selected-node construction
and lowering from broad owners into `cast_ops.cpp`/`cast_ops.hpp`, leaving
`instruction.cpp` family-neutral and `dispatch.cpp` as routing.

## Watchouts

- Preserve scalar cast behavior; this is ownership redistribution only.
- Do not expand cast semantics or rewrite expectations to claim progress.
- Preserve F128 delegation and fail-closed boundaries described by the shard.
- Keep `cast_ops.md` until its valid durable content has been reconciled.

## Proof

No build proof was run for this lifecycle-only activation.
