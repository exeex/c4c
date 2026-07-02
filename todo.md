Status: Active
Source Idea Path: ideas/open/538_rv64_object_global_address_helper_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map the global-address boundary

# Current Packet

## Just Finished

Lifecycle activation created the active runbook for Step 1.

## Suggested Next

Execute Step 1 from `plan.md`: map the global-address boundary, classify movable and parked helpers, record prerequisites, and preserve the delegated Step 2 proof command.

## Watchouts

- Do not move `append_rv64_prepared_data_objects`, final data section selection, global object symbol definition, data relocation assembly, text module assembly, ELF writing, or final object assembly.
- Do not change symbol spelling, visibility, relocation targets, object bytes, tests, unsupported markers, gcc_torture expectations, or data-object contracts.
- Keep local memory and global address helper ownership distinct.
- Keep routine execution notes in this file; do not edit the source idea unless durable source intent changes.

## Proof

No validation run for lifecycle-only activation.
