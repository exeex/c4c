Status: Active
Source Idea Path: ideas/open/202_route3_memory_source_identity_adapter.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select Reader And Baseline Proof

# Current Packet

## Just Finished

Lifecycle activation created `plan.md` and reset execution state for Step 1.

## Suggested Next

Execute Step 1 from `plan.md`: select exactly one direct memory/source identity
reader, identify retained prepared target-addressing fallback, and define the
narrow proof subset before code changes.

## Watchouts

Do not move address formation, frame/global/TLS relocation, stack/frame
offsets, concrete layout, addressing-mode legality, materialization, final
operands, or target-addressing fallback into BIR schema.

Do not replace all `memory_accesses`, `PreparedMemoryAccessLookups`, or
memory/frame/stack helper groups.

## Proof

Lifecycle-only activation; no build or test proof required.
