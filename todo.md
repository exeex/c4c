Status: Active
Source Idea Path: ideas/open/38_aarch64_module_compatibility_foldback_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Compatibility Projection Ownership

# Current Packet

## Just Finished

Plan activated from
`ideas/open/38_aarch64_module_compatibility_foldback_cleanup.md`.

## Suggested Next

Step 1 - Inventory Compatibility Projection Ownership: inspect
`compatibility_projection.*`, `module_compile.*`, include sites, call sites,
legacy projection consumers, and build metadata, then record the recommended
first mechanical fold-back packet in this file without implementation edits.

## Watchouts

Keep this route mechanical. Do not make `FunctionRecord::machine_nodes` a
terminal assembly authority, route new behavior through compatibility
projection records, rewrite final assembly emission, alter MIR stream
ownership, or mix in calls/dispatch/memory/comparison/prologue cleanup.

## Proof

No build required for activation-only lifecycle work.
